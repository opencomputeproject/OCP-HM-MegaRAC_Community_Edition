#!/usr/bin/env python3

# Contributors Listed Below - COPYRIGHT 2018
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import argparse
import os
import sh
import sys

git = sh.git.bake('--no-pager')


def log(msg, args):
    if args.noisy:
        sys.stderr.write('{}\n'.format(msg))


def git_clone_or_reset(local_name, remote, args):
    if not os.path.exists(local_name):
        log('cloning into {}...'.format(local_name), args)
        git.clone(remote, local_name)
    else:
        log('{} exists, updating...'.format(local_name), args)
        git.fetch(_cwd=local_name)
        git.reset('--hard', 'FETCH_HEAD', _cwd=local_name)


def extract_project_from_uris(uris):
    # remove SRC_URI = and quotes (does not handle escaped quotes)
    uris = uris.split('"')[1]
    for uri in uris.split():
        if 'github.com/openbmc' not in uri:
            continue

        # remove fetcher arguments
        uri = uri.split(';')[0]
        # the project is the right-most path segment
        return uri.split('/')[-1].replace('.git', '')

    return None


def extract_sha_from_recipe(recipe):
    with open(recipe) as fp:
        uris = ''
        project = None
        sha = None

        for line in fp:
            line = line.rstrip()
            if 'SRCREV' in line:
                sha = line.split('=')[-1].replace('"', '').strip()
            elif not project and uris or '_URI' in line:
                uris += line.split('\\')[0]
                if '\\' not in line:
                    # In uris we've gathered a complete (possibly multi-line)
                    # assignment to a bitbake variable that ends with _URI.
                    # Try to pull an OpenBMC project out of it.
                    project = extract_project_from_uris(uris)
                    if project is None:
                        # We didn't find a project.  Unset uris and look for
                        # another bitbake variable that ends with _URI.
                        uris = ''

            if project and sha:
                return (project, sha)

        raise RuntimeError('No SRCREV or URI found in {}'.format(recipe))


def find_candidate_recipes(meta, args):
    remote_fmt_args = (args.ssh_config_host, meta)
    remote = 'ssh://{}/openbmc/{}'.format(*remote_fmt_args)
    try:
        git_clone_or_reset(meta, remote, args)
    except sh.ErrorReturnCode as e:
        log('{}'.format(e), args)
        return []

    grep_args = ['-l', '-e', '_URI', '--and', '-e', 'github.com/openbmc']
    try:
        return git.grep(*grep_args, _cwd=meta).stdout.decode('utf-8').split()
    except sh.ErrorReturnCode_1:
        pass
    except sh.ErrorReturnCode as e:
        log('{}'.format(e), args)

    return []


def find_and_process_bumps(meta, args):
    candidate_recipes = find_candidate_recipes(meta, args)

    for recipe in candidate_recipes:
        full_recipe_path = os.path.join(meta, recipe)
        recipe_basename = os.path.basename(full_recipe_path)
        project_name, recipe_sha = extract_sha_from_recipe(full_recipe_path)

        remote_fmt_args = (args.ssh_config_host, project_name)
        remote = 'ssh://{}/openbmc/{}'.format(*remote_fmt_args)
        ls_remote_args = [remote, 'refs/heads/{}'.format(args.branch)]
        try:
            project_sha = git('ls-remote', *ls_remote_args)
            project_sha = project_sha.stdout.decode('utf-8').split()[0]
        except sh.ErrorReturnCode as e:
            log('{}'.format(e), args)
            continue

        if project_sha == recipe_sha:
            message_args = (recipe_basename, recipe_sha[:10])
            print('{} is up to date ({})'.format(*message_args))
            continue

        change_id = 'autobump {} {} {}'.format(recipe, recipe_sha, project_sha)
        hash_object_args = ['-t', 'blob', '--stdin']
        change_id = git(sh.echo(change_id), 'hash-object', *hash_object_args)
        change_id = 'I{}'.format(change_id.strip())

        query_args = ['query', 'change:{}'.format(change_id)]
        gerrit_query_result = args.gerrit(*query_args)
        gerrit_query_result = gerrit_query_result.stdout.decode('utf-8')

        if (change_id in gerrit_query_result):
            message_args = (recipe_basename, change_id)
            print('{} {} already exists'.format(*message_args))
            continue

        message_args = (recipe_basename, recipe_sha[:10], project_sha[:10])
        print('{} updating from {} to {}'.format(*message_args))

        remote_args = (args.ssh_config_host, project_name)
        remote = 'ssh://{}/openbmc/{}'.format(*remote_args)
        git_clone_or_reset(project_name, remote, args)

        try:
            revlist = '{}..{}'.format(recipe_sha, project_sha)
            shortlog = git.shortlog(revlist, _cwd=project_name)
            shortlog = shortlog.stdout.decode('utf-8')
        except sh.ErrorReturnCode as e:
            log('{}'.format(e), args)
            continue

        reset_args = ['--hard', 'origin/{}'.format(args.branch)]
        git.reset(*reset_args, _cwd=meta)

        recipe_content = None
        with open(full_recipe_path) as fd:
            recipe_content = fd.read()

        recipe_content = recipe_content.replace(recipe_sha, project_sha)
        with open(full_recipe_path, 'w') as fd:
            fd.write(recipe_content)

        git.add(recipe, _cwd=meta)

        commit_summary_args = (project_name, recipe_sha[:10], project_sha[:10])
        commit_msg = '{}: srcrev bump {}..{}'.format(*commit_summary_args)
        commit_msg += '\n\n{}'.format(shortlog)
        commit_msg += '\n\nChange-Id: {}'.format(change_id)

        git.commit(sh.echo(commit_msg), '-s', '-F', '-', _cwd=meta)

        push_args = ['origin', 'HEAD:refs/for/{}/autobump'.format(args.branch)]
        if not args.dry_run:
            git.push(*push_args, _cwd=meta)


def main():
    app_description = '''OpenBMC bitbake recipe bumping tool.

Find bitbake metadata files (recipes) that use the git fetcher
and check the project repository for newer revisions.

Generate commits that update bitbake metadata files with SRCREV.

Push generated commits to the OpenBMC Gerrit instance for review.
    '''
    parser = argparse.ArgumentParser(
        description=app_description,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.set_defaults(branch='master')
    parser.add_argument(
        '-d', '--dry-run', dest='dry_run', action='store_true',
        help='perform a dry run only')
    parser.add_argument(
        '-m', '--meta-repository', dest='meta_repository', action='append',
        help='meta repository to check for updates')
    parser.add_argument(
        '-v', '--verbose', dest='noisy', action='store_true',
        help='enable verbose status messages')
    parser.add_argument(
        'ssh_config_host', metavar='SSH_CONFIG_HOST_ENTRY',
        help='SSH config host entry for Gerrit connectivity')

    args = parser.parse_args()
    setattr(args, 'gerrit', sh.ssh.bake(args.ssh_config_host, 'gerrit'))

    metas = getattr(args, 'meta_repository')
    if metas is None:
        metas = args.gerrit('ls-projects', '-m', 'meta-')
        metas = metas.stdout.decode('utf-8').split()
        metas = [os.path.split(x)[-1] for x in metas]

    for meta in metas:
        find_and_process_bumps(meta, args)


if __name__ == '__main__':
    sys.exit(0 if main() else 1)

#import git
import os
import yaml
import re
import sys


'''
Get File Content utility function to get local file content
@params prj - path to directory containing the project
@params config_path - file name / path to config file
@return fc - file content of config file
'''
def get_file_content(prj, config_path):
    #print(os.path.join(prj, config_path))
    with open(os.path.join(prj, config_path), 'r') as f:
        fc = f.read()
    return fc

'''
Empty layer global initialization
'''
layer = []


'''
Recursive function to scan and get all config files from root file
@params prj - path to directory containing the project
@params config_path - file name / path to config file
@return configs - Combined configuration file after recursively scanning the directory
'''
def scan_and_get_all_configs(prj, config_path, parent_feature_enabled=True):
    #print(config_path)
    if parent_feature_enabled:
        try:
            configs = yaml.safe_load(get_file_content(prj, config_path))
            #print(configs)
        except Exception as e:
            print(e)
            configs = {}
    
        if not configs:
            # print(configs.keys())
        # else:
            configs = {}
    
        if 'Features' in configs.keys():
            # print(type(configs['Features']))
            '''
            Root level config is of type dict to categorize feature list
            Most sub level configs are of type list
            '''
            if type(configs['Features']) == dict:
                for features in configs['Features'].keys():
                    for feature in configs['Features'][features]:
                        '''
                        Expect a path key in every feature. Path can be a .yaml or .bb file
                        '''
                        if 'path' in feature.keys() and feature['path']:
                            '''
                            If path is of type .yaml then it will contain some subfeatures or configurations
                                get the content of the .yaml in a recursive call and store in new key `content`
                                the parent folder name represents the feature
                            If path is of type .bb then it won't contain any subfeatures or configurations
                                The filename itself represents the feature
                            If feature is enabled then add to layer list for final construct
                            '''
                            if feature['path'].endswith('.yaml'):
                                feature['content'] = scan_and_get_all_configs(prj, feature['path'], parent_feature_enabled and feature['enable'])
                                if parent_feature_enabled and feature['enable']:
                                    layer.append(feature['path'].split('/').pop(-2))
                            elif feature['path'].endswith('.bb'):
                                if parent_feature_enabled and feature['enable']:
                                    layer.append(feature['path'].split('/').pop().replace('.bb',''))
    
    
            elif type(configs['Features']) == list:
                for feature in configs['Features']:
                    '''
                    Expect a path key in every feature. Path can be a .yaml or .bb file
                    '''
                    if 'path' in feature.keys() and feature['path']:
                        '''
                        If path is of type .yaml then it will contain some subfeatures or configurations
                            get the content of the .yaml in a recursive call and store in new key `content`
                            the parent folder name represents the feature
                        If path is of type .bb then it won't contain any subfeatures or configurations
                            The filename itself represents the feature
                        If feature is enabled then add to layer list for final construct
                        '''
                        if feature['path'].endswith('.yaml'):
                            feature['content'] = scan_and_get_all_configs(prj, feature['path'], parent_feature_enabled and feature['enable'])
                            if parent_feature_enabled and feature['enable']:
                                layer.append(feature['path'].split('/').pop(-2))
                        elif feature['path'].endswith('.bb'):
                            if parent_feature_enabled and feature['enable']:
                                layer.append(feature['path'].split('/').pop().replace('.bb',''))
    else :
        configs={}
    return configs


res = scan_and_get_all_configs('.', sys.argv[2])

def get_features_configs(configs):
    f = {}
    c = {}
    if 'Features' in configs.keys():
       # print(type(configs['Features']))
        if type(configs['Features']) == dict:
            for features in configs['Features'].keys():
                for feature in configs['Features'][features]:
                    '''
                    All feature must have an spx_macro, otherwise throw a Warning
                    If it is present, copy the state of it so we can update core_features and core_macros
                    '''
                    if 'spx_macro' in feature.keys():
                        # print('dict Feature:',feature['spx_macro'],feature['enable'])
                        f[feature['spx_macro']] = feature['enable']
		    elif 'macro' in feature.keys():
			f[feature['macro']] = feature['enable']
                    else:
                        print("Warning, ", feature['name']," this feature is not proper")
                    if 'path' in feature.keys() and feature['path'] and feature['path'].endswith('.yaml'):
                        '''
                        Get the feature configuration recursively, if file is of type yaml and update features and configurations
                        '''
                        f1,c1 = get_features_configs(feature['content'])
                        f.update(f1)
                        c.update(c1)
        elif type(configs['Features']) == list:
            for feature in configs['Features']:
                if 'spx_macro' in feature.keys():
                    # print('list Feature:', feature['spx_macro'], feature['enable'])
                    f[feature['spx_macro']] = feature['enable']
		elif 'macro' in feature.keys():
                        f[feature['macro']] = feature['enable']
                else:
                    print("Warning, ", feature['name']," this feature is not proper")
                if 'path' in feature.keys() and feature['path'] and feature['path'].endswith('.yaml'):
                    f1,c1 = get_features_configs(feature['content'])
                    # print("f1:", f1,"c1:",c1)
                    f.update(f1)
                    c.update(c1)

    if 'Configurations' in configs.keys():
        if type(configs['Configurations']) == list:
            '''
            Update all the configurations in config table to update core_macros
            '''
            for config in configs['Configurations']:
                if 'spx_macro' in config.keys():
                    # print('Macro:', config['spx_macro'], config['value'])
                    c[config['spx_macro']] = config['value']
                elif 'spx_mapping' in config.keys() and 'allowed_values' in config.keys():
                    avi=0
                    for av in config['allowed_values']:
                        if av == config['value']:
                            c[config['spx_mapping'][avi]] = True
                        else:
                            c[config['spx_mapping'][avi]] = False
                        avi = avi+1
                elif 'macro' in config.keys():
                    # print('Macro:', config['macro'], config['value'])
                    c[config['macro']] = config['value']
                elif 'mapping' in config.keys() and 'allowed_values' in config.keys():
                    avi=0
                    for av in config['allowed_values']:
                        if av == config['value']:
                            c[config['mapping'][avi]] = True
                        else:
                            c[config['mapping'][avi]] = False
                        avi = avi+1
                else:
                    print("Warning, ", config," this config is not proper")

    return f,c

features, configs = get_features_configs(res)

# print(features, configs)

def remove_feature_item(ctnt, f):
#    print("Rmove:" + f )
    return re.sub(f+"(.*?)\n","",ctnt)

# create pattern of '^XXX^'
pat = re.compile('\^[\w]+\^')
def update_prjdef_header(ctnt, f, v):
    
    if type(v) == bool:
        if v == True:
            val = "YES"
        else:
            val = "NO"
    elif type(v) == int:
        val = hex(v)
        m = re.search(f + " ([^\n]+)", ctnt)
        if m:
            tval = m.group(1)
            if '"' in tval:
                val = "\""+str(v)+"\""
            else:
                val = hex(v)
    else:
        val = "\""+str(v)+"\""

    if type(val) == str and val.upper() == "NO":
        return remove_feature_item(ctnt, "#define " + f)
    else:
        tmp_ctnt = re.subn("#define " + f + " [^\n]+","#define " + f + " " + str(val) , ctnt)
    if val != "NO" and tmp_ctnt[1] == 0 and not pat.search(f):
        # print("header:" , f,"added")
        ctnt = ctnt + "#define " + f + " " + str(val) + "\n"
        return ctnt
    return tmp_ctnt[0]

def update_prjdef_make(ctnt, f, v):

    if type(v) == bool:
        if v == True:
            val = "YES"
        else:
            val = "NO"
    else:
        val = "\""+str(v)+"\""

    if type(val) == str and val.upper() == "NO":
        return remove_feature_item(ctnt, f)
    else:
        tmp_ctnt = re.subn(f + "=[^\n]+", f + "=" + str(val) , ctnt)
    if tmp_ctnt[1] == 0 and not pat.search(f):
        # print("mk :" , f,"added")
        ctnt = ctnt + f + "=" + str(val) + "\n"
        return ctnt
    return tmp_ctnt[0]

def update_prjdef_cfg(ctnt, f, v):
    val = v

    if type(v) == bool:
        if v == True:
            val = "YES"
        else:
            val = "NO"
    
    if type(val) == str and val.upper() == "NO":
        return remove_feature_item(ctnt, f)
    else:
        tmp_ctnt = re.subn(f + "=[^\n]+", f + "=\"" + str(val) + "\"", ctnt)
    if tmp_ctnt[1] == 0 and not pat.search(f):
        # print("cfg:", f,"added")
        ctnt = ctnt + f + "=\"" + str(val) + "\"\n"
        return ctnt
    return tmp_ctnt[0]
    

def update_core_macro(ctnt, f, v):

    if type(v) == bool:
        if v == True:
            val = "YES"
        else:
            val = "NO"
    elif type(v) == int:
        val = hex(v)
    else:
        val = str(v)

    if type(val) == str and val.upper() == "NO":
        return remove_feature_item(ctnt, f)
    else:
        tmp_ctnt = re.subn(f + "=[^\n]+", f + "=" + str(val) , ctnt)
    if tmp_ctnt[1] == 0 and not pat.search(f):
        # print("macro:", f,"added")
        ctnt = ctnt + f + "=" + str(val) + "\n"
        return ctnt
    return tmp_ctnt[0]

def update_core_features(ctnt, f, v):
    pass

pdc_ctnt = ''
pdh_ctnt = ''
pdmk_ctnt = ''
with open('projdef.cfg', 'r') as pdf:
    pdc_ctnt = pdf.read()
for f,v in features.iteritems():
    pdc_ctnt = update_prjdef_cfg(pdc_ctnt, f, v)

for c,v in configs.iteritems():
    pdc_ctnt = update_prjdef_cfg(pdc_ctnt, c, v)

with open('projdef.mod.cfg', 'w') as pdf:
    pdf.write(pdc_ctnt)

final_layer = list(set(layer) - set(['recipes-osp']))
DELIMIT = "\n## DO NOT EDIT BELOW - ADDED BY DEVNET\n"
iia = DELIMIT
iia += "IMAGE_INSTALL_append += \"" + ' '.join(final_layer) + "\""
    
this_metalayer = sys.argv[1]

with open(this_metalayer + '/layer.conf', 'r') as lf:
    clayer = lf.read()

clayer = clayer.split(DELIMIT)[0]


with open(this_metalayer + '/layer.conf', 'w') as lf:
    lf.write(clayer+iia)

if this_metalayer.find("ami") == -1:
    sys.exit() 

DELIMIT = "\n## DO NOT EDIT BELOW - ADDED BY SCRIPT\n"
machine = this_metalayer.split("/")
new_machine = DELIMIT
new_machine += "BBLAYERS += " "\"" + "##OEROOT##/meta-ami/" + machine[1] + "\""

with open(this_metalayer + '/bblayers.conf.sample','r') as lf:
   bblayer = lf.read()
   bblayer = bblayer.split(DELIMIT)[0]

with open(this_metalayer + '/bblayers.conf.sample','w') as lf:
    lf.write(bblayer+new_machine)


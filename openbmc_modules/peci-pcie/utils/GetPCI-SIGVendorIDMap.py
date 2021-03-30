#!/usr/bin/python3
import sys
import requests
import re
import argparse
from bs4 import BeautifulSoup


def parse_args(argv):
    """Parse the command-line arguments"""
    parser = argparse.ArgumentParser(description='Get the PCI-SIG Vendor IDs')
    parser.add_argument('--http-proxy', action='store', help="HTTP Proxy Address")
    parser.add_argument('--https-proxy', action='store', help="HTTPS Proxy Address")
    args = parser.parse_args(argv)
    return args


def main(argv):
    """Go to the PCI-SIG members page and construct a
    dictionary of member companies to their Vendor IDs"""
    args = parse_args(argv)

    proxyDict = {
        "http": args.http_proxy,
        "https": args.https_proxy
    }
    page = 'https://pcisig.com/membership/member-companies'
    pciVendorIDs = {}
    while True:
        r = requests.get(page, proxies=proxyDict)
        soup = BeautifulSoup(r.text)

        for row in soup.table.tbody.find_all("tr"):
            fields = row.find_all("td")
            vendorID = fields[1].text.strip()
            if 'hex' in vendorID.lower():
                match = re.match(r'\w+ \((\w+) hex\)', vendorID, re.I)
                if match is not None:
                    vendorID = match.group(1)
                else:
                    vendorID = ''
            if vendorID != '':
                vendorName = fields[0].text.replace('"', '').strip()
                pciVendorIDs[vendorName] = vendorID

        page = soup.find("a", title="Go to next page")
        if page is None:
            break
        page = 'https://pcisig.com' + page["href"]

    for name, vid in sorted(pciVendorIDs.items(), key=lambda x: x[0].lower()):
        print("{{0x{}, \"{}\"}},".format(vid, name))


if __name__ == '__main__':
    main(sys.argv[1:])

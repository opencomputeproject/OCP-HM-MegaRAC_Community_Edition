#!/usr/bin/env python3

"""
This script applies patches to an XML file.

The patch file is itself an XML file.  It can have any root element name,
and uses XML attributes to specify if the elements in the file should replace
existing elements or add new ones.  An XPath attribute is used to specify
where the fix should be applied.  A <targetFile> element is required in the
patch file to specify the base name of the XML file the patches should be
applied to, though the targetFile element is handled outside of this script.

The only restriction is that since the type, xpath, and key attributes are
used to specify the patch placement the target XML cannot use those at a
top level element.

 It can apply patches in 5 ways:

 1) Add an element:
    Put in the element to add, along with the type='add' attribute
    and an xpath attribute specifying where the new element should go.

     <enumerationType type='add' xpath="./">
       <id>MY_TYPE</id>
     </enumerationType>

     This will add a new enumerationType element child to the root element.

 2) Replace an element:
    Put in the new element, with the type='replace' attribute
    and the XPath of the element you want to replace.

     <enumerator type='replace'
               xpath="enumerationType/[id='TYPE']/enumerator[name='XBUS']">
       <name>XBUS</name>
       <value>the new XBUS value</value>
     </enumerator>

    This will replace the enumerator element with name XBUS under the
    enumerationType element with ID TYPE.

 3) Remove an element:
    Put in the element to remove, with the type='remove' attribute and
    the XPath of the element you want to remove. The full element contents
    don't need to be specified, as the XPath is what locates the element.

    <enumerator type='remove'
                xpath='enumerationType[id='TYPE]/enumerator[name='DIMM']>
    </enumerator>

    This will remove the enumerator element with name DIMM under the
    enumerationType element with ID TYPE.

 4) Add child elements to a specific element.  Useful when adding several
    child elements at once.

    Use a type attribute of 'add-child' and specify the target parent with
    the xpath attribute.

     <enumerationType type="add-child" xpath="enumerationType/[id='TYPE']">
       <enumerator>
         <name>MY_NEW_ENUMERATOR</name>
         <value>23</value>
       </enumerator>
       <enumerator>
         <name>ANOTHER_NEW_ENUMERATOR</name>
         <value>99</value>
       </enumerator>
     </enumerationType>

     This will add 2 new <enumerator> elements to the enumerationType
     element with ID TYPE.

 5) Replace a child element inside another element, useful when replacing
    several child elements of the same parent at once.

    Use a type attribute of 'replace-child' and the xpath attribute
    as described above, and also use the key attribute to specify which
    element should be used to match on so the replace can be done.

     <enumerationType type="replace-child"
                      key="name"
                      xpath="enumerationType/[id='TYPE']">
       <enumerator>
         <name>OLD_ENUMERATOR</name>
         <value>newvalue</value>
       </enumerator>
       <enumerator>
         <name>ANOTHER_OLD_ENUMERATOR</name>
         <value>anothernewvalue</value>
       </enumerator>
     </enumerationType>

     This will replace the <enumerator> elements with the names of
     OLD_ENUMERATOR and ANOTHER_OLD_ENUMERATOR with the <enumerator>
     elements specified, inside of the enumerationType element with
     ID TYPE.
"""


from lxml import etree
import sys
import argparse


def delete_attrs(element, attrs):
    for a in attrs:
        try:
            del element.attrib[a]
        except:
            pass

if __name__ == '__main__':

    parser = argparse.ArgumentParser("Applies fixes to XML files")
    parser.add_argument("-x", dest='xml', help='The input XML file')
    parser.add_argument("-p", dest='patch_xml', help='The patch XML file')
    parser.add_argument("-o", dest='output_xml', help='The output XML file')
    args = parser.parse_args()

    if not all([args.xml, args.patch_xml, args.output_xml]):
        parser.print_usage()
        sys.exit(-1)

    errors = []
    patch_num = 0
    patch_tree = etree.parse(args.patch_xml)
    patch_root = patch_tree.getroot()
    tree = etree.parse(args.xml)
    root = tree.getroot()

    for node in patch_root:
        if (node.tag is etree.PI) or (node.tag is etree.Comment) or \
           (node.tag == "targetFile"):
            continue

        patch_num = patch_num + 1

        xpath = node.get('xpath', None)
        patch_type = node.get('type', 'add')
        patch_key = node.get('key', None)
        delete_attrs(node, ['xpath', 'type', 'key'])

        print("Patch " + str(patch_num) + ":")

        try:
            if xpath is None:
                raise Exception("  E>  No XPath attribute found")

            target = tree.find(xpath)

            if target is None:
                raise Exception("  E>  Could not find XPath target " + xpath)

            if patch_type == "add":

                print("  Adding element " + target.tag + " to " + xpath)

                #The ServerWiz API is dependent on ordering for the
                #elements at the root node, so make sure they get appended
                #at the end.
                if (xpath == "./") or (xpath == "/"):
                    root.append(node)
                else:
                    target.append(node)

            elif patch_type == "remove":

                print("  Removing element " + xpath)
                parent = target.find("..")
                if parent is None:
                    raise Exception("  E>  Could not find parent of " + xpath +
                                    " so can't remove this element")
                parent.remove(target)

            elif patch_type == "replace":

                print("  Replacing element " + xpath)
                parent = target.find("..")
                if parent is None:
                    raise Exception("  E>  Could not find parent of " + xpath +
                                    " so can't replace this element")
                parent.remove(target)
                parent.append(node)

            elif patch_type == "add-child":

                for child in node:
                    print("  Adding a '" + child.tag + "' child element"
                          " to " + xpath)
                    target.append(child)

            elif patch_type == "replace-child":

                if patch_key is None:
                    raise Exception("  E>  Patch type is replace-child, but"
                                    " 'key' attribute isn't set")
                updates = []
                for child in node:
                    #Use the key to figure out which element to replace
                    key_element = child.find(patch_key)
                    for target_child in target:
                        for grandchild in target_child:
                            if (grandchild.tag == patch_key) and \
                               (grandchild.text == key_element.text):
                                update = {}
                                update['remove'] = target_child
                                update['add'] = child
                                updates.append(update)

                for update in updates:
                    print("  Replacing a '" + update['remove'].tag +
                          "' element in path " + xpath)
                    target.remove(update['remove'])
                    target.append(update['add'])

            else:
                raise Exception("  E>  Unknown patch type attribute found:  " +
                                patch_type)

        except Exception as e:
            print(e)
            errors.append(e)

    tree.write(args.output_xml)

    if errors:
        print("Exiting with " + str(len(errors)) + " total errors")
        sys.exit(-1)

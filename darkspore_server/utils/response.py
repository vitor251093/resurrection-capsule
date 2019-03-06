import json
import xml.etree.cElementTree as xml_tree
from xml.etree import ElementTree

from flask import Response

def jsonResponseWithObject(obj):
    json_data = json.dumps(obj)
    return Response(json_data, mimetype='application/json')

def xmlResponseWithObject(obj):
    xml = xml_tree.Element("response")
    for key,val in obj.items():
        setXmlValues(xml, key, val)
    tree_str = ElementTree.tostring(xml, encoding='iso-8859-1', method='xml')
    return Response(tree_str, mimetype='text/xml')


def setXmlValues(xml, key, values):
    if values is None:
        xml_tree.SubElement(xml, key)
        return

    if isinstance(values, list):
        if len(values) == 0:
            xml_tree.SubElement(xml, key)
            return

        for val in values:
            setXmlValues(xml, key, val)
        return

    if isinstance(values, dict):
        sub = xml_tree.SubElement(xml, key)
        for sub_key,val in values.items():
            setXmlValues(sub, sub_key, val)
        return

    xml_tree.SubElement(xml, key).text = str(values)

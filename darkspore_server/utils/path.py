import os
import pickle

def join(comp1, comp2):
    if os.name == 'nt':
        while comp2.startswith('..'):
            comp1 = os.path.dirname(comp1)
            comp2 = comp2[3:]
        return os.path.join(comp1.replace("/","\\"), comp2.replace("/","\\"))
    return os.path.join(comp1.replace("\\","/"), comp2.replace("\\","/"))

def loadObjectFromFile(file):
    if os.path.isfile(file):
        with open(file, 'rb') as config_user_file:
            return pickle.load(config_user_file)
    return None

def saveObjectToFile(file, obj):
    with open(file, 'wb') as config_user_file:
        pickle.dump(obj, config_user_file)

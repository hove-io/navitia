 #!/usr/bin/python
# -*- coding: utf-8 -*-

jmeter_result = "jmeter.results"

import subprocess
import sys
import os
import shutil
import time
import xml.etree.ElementTree as ET

if len(sys.argv) != 2:
    print("Utilisation : ", sys.argv[0], " depot_navitia")
    sys.exit(1)

print("Clone du dépot")
shutil.rmtree("navitia_git", ignore_errors=True)
res = subprocess.call(["git", "clone", "--recursive", sys.argv[1], "navitia_git"])
if res != 0:
    print "Il y a eu un soucis lors du clone, code retour : ", str(res)
    sys.exit(1)
print

print("Création d'un repertoire build")
shutil.rmtree("build", ignore_errors=True)
os.mkdir("build")
os.chdir("build")
print

print("Cmake")
res = subprocess.call(["cmake", "-DCMAKE_BUILD_TYPE=Release", "../navitia_git/source/"])
if res != 0:
    print"Il y a eu des soucis avec cmake, code retour : ", str(res)
    sys.exit(1)
print

print("Compilation")
res = subprocess.call(["make", "-j8"])
if res != 0:
    print"Il y a eu des soucis avec la compilation, code retour : ", str(res)
    sys.exit(1)
print

print "Tests unitaires"
res = subprocess.call(["make", "test"])
if res != 0:
    print "Il y a eu des soucis avec les tests unitaires, code retour : ", str(res)
    sys.exit(1)
print

os.chdir("..")

print "Binarisation de Nantes"
res = subprocess.call(["./build/naviMake/navimake", "--osm", "./data_sources/Nantes/nantes.pbf", "-i", "./data_sources/Nantes/", "-o", "nantes.nav.lz4"])
if res != 0:
    print "Il y a eu des soucis avec la binarisation, code retour : ", str(res)
    sys.exit(1)
print

print "Création du fichier de configuration Navitia"
f = open('navitia.ini', 'w+') # ça tronque le fichier
f.write(
"""[GENERAL]
database=nantes.nav.lz4""")
f.close() # ça tronque le fichier

print "Instanciation de navitia"
navitia = subprocess.Popen(["./build/navitia/navitia"])
#On laisse le temps que les données soient effectivement chargée
time.sleep(5)
navitia.poll()
if navitia.returncode != None:
    print "Impossible d'instancier navitia, code de retour : ", str(navitia.returncode)
print


print "Création du fichier de configuration Jörmungandr"
f = open('Jörmungandr.ini', 'w+') # ça tronque le fichier
f.write(
"""[instance]
socket=ipc:///tmp/default_navitia
key=moo""")
f.close()

print "Instanciation de Jörmungandr"
jormungandr = subprocess.Popen(["python", "./navitia_git/source/frontend/navitia.py"])
#On laisse le temps de se charger doucement
time.sleep(1)
jormungandr.poll()
if jormungandr.returncode != None:
    print "Impossible d'instancier Jörmungandr, code de retour : ", str(navitia.jormungandr)
print


print "Lancement des tests jmeter"
if os.path.isfile(jmeter_result):
    os.remove(jmeter_result)
res = subprocess.call(["jmeter", "-n", "-t", "./navitia_git/jmeter/api_tests.jmx", "-l", jmeter_result])
if res != 0:
    print"Avec jmeter : ", str(res)
print

navitia.kill()
jormungandr.kill()

tree = ET.parse("jmeter.results")

for httpSample in tree.iter("httpSample"):
    for assertionResult in httpSample.iter("assertionResult"):
        if assertionResult.find('failure').text == 'true':
            print "Problème pour l'assertion", httpSample.attrib['lb'], "avec le message", assertionResult.find('failureMessage').text

import os
from os import path
import json
from shutil import copyfile
Import("env")

def generatejson(target, source, env):
    source = os.path.join(env.get('PROJECT_BUILD_DIR'), env.get('PIOENV'), env["PROGNAME"] + '.bin')

    if not os.path.exists(source):
        raise Exception("Source file not found")

    avrfolder = os.path.join(env.get('PROJECT_DIR'), '..', 'ESPController', 'data', 'avr')

    if not os.path.exists(avrfolder):
        os.mkdir(avrfolder)

    my_flags = env.ParseFlags(env['BUILD_FLAGS'])
    defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}

    newfilename = "fw_%s_%s.bin" % (env["PIOENV"], env["git_sha_short"])

    if len(newfilename) > 35:
        raise Exception("Generated file name is longer than 35 chars")

    copyfile(source, os.path.join(avrfolder, newfilename))

    manifestjson = os.path.join(avrfolder, 'manifest.json')

    data = {}

    if os.path.exists(manifestjson):
        with open(manifestjson, 'r') as json_file:
            data = json.load(json_file)

    signature = ""
    board = str(env["PIOENV"]).upper()

    if 'avrprog' not in data:
        data['avrprog'] = []

    if str(env["BOARD_MCU"]).lower() == "attiny841":
        signature = "1e9315"

    if str(env["BOARD_MCU"]).lower() == "attiny1614":
        signature = "1e9422"

    if str(env["BOARD_MCU"]).lower() == "attiny1624":
        signature = "1e942a"
        
    if signature == "":
        raise Exception("Unknown chip signature")

    for i in range(len(data['avrprog'])): 
        if data['avrprog'][i]['board'] == board: 
            del data['avrprog'][i] 
            break

    if str(env["BOARD_MCU"]).lower() == "attiny1624":
        data['avrprog'].append({
            'board': board, 'name': newfilename, 'ver': env["git_sha_short"],
            'mcu': signature, 'efuse': 0, 'hfuse': 0, 'lfuse': 0
        })

    if str(env["BOARD_MCU"]).lower() == "attiny824":
        data['avrprog'].append({
            'board': board, 'name': newfilename, 'ver': env["git_sha_short"],
            'mcu': signature, 'efuse': 0, 'hfuse': 0, 'lfuse': 0
        })

    if str(env["BOARD_MCU"]).lower() == "attiny841":
        efuse = hex(int(env.GetProjectOption("board_fuses.efuse"), 2)).upper()[2:4]
        hfuse = hex(int(env.GetProjectOption("board_fuses.hfuse"), 2)).upper()[2:4]
        lfuse = hex(int(env.GetProjectOption("board_fuses.lfuse"), 2)).upper()[2:4]

        data['avrprog'].append({
            'board': board, 'name': newfilename, 'ver': env["git_sha_short"],
            'mcu': signature, 'efuse': efuse, 'hfuse': hfuse, 'lfuse': lfuse
        })

    with open(manifestjson, 'w') as outfile:
        json.dump(data, outfile, indent=4, sort_keys=True)

env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.hex",
    env.VerboseAction(
        " ".join([
            "$OBJCOPY", "-I", "ihex", 
            os.path.join("$BUILD_DIR", "${PROGNAME}.hex"), 
            "-O", "binary", 
            os.path.join("$BUILD_DIR", "${PROGNAME}.bin")
        ]), 
        "Building binary file $BUILD_DIR/${PROGNAME}.bin"
    )
)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", generatejson)

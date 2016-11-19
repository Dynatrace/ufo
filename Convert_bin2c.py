import gzip
import shutil
import sys
import binascii
import cStringIO
import io
import os



def convert_file(file, output):
    try:
        f = open(file, "rb")
        binary = f.read()
        if len(binary) > 0:
            output.write('static const char PROGMEM ')

            index = file.rfind('/')
            if index == -1:
                index = file.rfind('\\')
            if index >= 0:
                file = file[index+1:]
            output.write(file.replace('.', '_'))
            output.write('[')
            output.write(str(len(binary)))
            output.write('] = {\n')

            if sys.version_info[0] >= 3:
                text = str(binascii.hexlify(binary), 'UTF-8')
            else:
                text = str(binascii.hexlify(binary))

            output.write('0x')
            output.write(text[0:2])
            i = 2
            while i < len(text):
                output.write(', ')
                if i % 100 == 0:
                    output.write('\n')
                output.write('0x')
                output.write(text[i:i+2])
                i += 2
            output.write('\n};\n\n')

    except IOError:
        print("cannot open input file %s" % file)

if len(sys.argv) <= 1:
    print('usage: python.exe convert.py <file1> <file2> ...')
    print('creates the file WebContent.h in the current directory containing the binary array representation of all provided files')

    exit()


try: 
    src = open("data/index.html", "rb") 
    gzf = gzip.GzipFile(filename="index.html.gz", mode='wb')
    gzf.write(src.read())
    src.close()
    gzf.close()
    
except IOError:
    print("cannot open index.html for reading or index.html.gz for writing")

try:
    output = open('WebContent.h', "w")
    convert_file("index.html.gz", output)
    for arg in sys.argv[1:]:
        fobj = open(arg, "rb")
        convert_file(arg, output)
    output.close()
except IOError:
    print("cannot open WebContent.h for writing")

#cleanup
os.remove("index.html.gz")


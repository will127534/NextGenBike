from PIL import Image
import numpy as np


def load_image( infilename ) :
    img = Image.open( infilename )
    img.load()
    img=img.resize((80,80))
    img.save("resize.png")
    image = img.convert("RGB")
    data=np.array(image)
    # print(data)
    return data
f=open("image.h","w")
def print_as_c_array(name,data):
    
    f.write("uint8_t %s[360][35] = {" % name)
    for degree in range(0, 360):
        f.write("{")
        for led in range(0, 35):
            if led == 34:
                f.write("%d" % data[led][degree])
            else:
                f.write("%d," % data[led][degree])
        if degree == 359:
            f.write("}\n")
        else:
            f.write("},\n")
    f.write("};")


Red = np.zeros((35,360))
Blue = np.zeros((35,360))
Green = np.zeros((35,360))
data=load_image("nyan.png")
for degree in range(360):
    for r in range(35):
        x = int(np.round(40 + (3 + r) * np.sin(degree / 180 * np.pi)))
        y = int(np.round(40 - (3 + r) * np.cos(degree / 180 * np.pi)))
        print("X:%s,Y:%s" % (x, y))
        data_R = data[x][y][0]
        data_G = data[x][y][1]
        data_B = data[x][y][2]

        Red[r][degree] = data_R
        Green[r][degree] = data_G
        Blue[r][degree] = data_B

# print(Red)
# print(Green)
# print(Blue)

print_as_c_array("PIC1_Red",Red)
print_as_c_array("PIC1_Green",Green)
print_as_c_array("PIC1_Blue",Blue)

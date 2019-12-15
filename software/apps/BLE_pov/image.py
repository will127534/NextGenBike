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
    
<<<<<<< HEAD
    f.write("static const uint8_t %s[360][34] = {" % name)
    for degree in range(360):           
        f.write("{")
        for led in range(34):
            if led == 33:
=======
    f.write("uint8_t %s[360][35] = {" % name)
    for degree in range(360):           
        f.write("{")
        for led in range(35):
            if led == 34:
>>>>>>> 99411e0d0fe281464589e79d9fb00f8538c551b7
                f.write("%d" % data[degree][led])
            else:
                f.write("%d," % data[degree][led])       
        if degree == 359:
            f.write("}\n")
        else:
            f.write("},\n")
<<<<<<< HEAD
    f.write("};\n")


image_list = ["left.png","right.png"]


for image_path in image_list:
    Red = np.zeros((360,34))
    Blue = np.zeros((360,34))
    Green = np.zeros((360,34))
    data=load_image(image_path)
    print(data.shape)


    for degree in range(360):  
        for r in range(34): 
            x = int(np.round(40 - (3 + r) * np.sin(degree / 180 * np.pi)))
            y = int(np.round(40 - (3 + r) * np.cos(degree / 180 * np.pi)))
            print("X:%s,Y:%s  " % (x, y),end="")
            data_R = data[y][x][0]
            data_G = data[y][x][1]
            data_B = data[y][x][2]


            Red[degree][r] = data_R
            Green[degree][r] = data_G
            Blue[degree][r] = data_B
        print()

    # print(Red)
    # print(Green)
    # print(Blue)

    print_as_c_array(image_path.split(".")[0] + "_R",Red)
    print_as_c_array(image_path.split(".")[0] + "_G",Green)
    print_as_c_array(image_path.split(".")[0] + "_B",Blue)
=======
    f.write("};")


Red = np.zeros((360,35))
Blue = np.zeros((360,35))
Green = np.zeros((360,35))
data=load_image("right.png")
print(data.shape)


for degree in range(360):  
    for r in range(35): 
        x = int(np.round(40 - (3 + r) * np.sin(degree / 180 * np.pi)))
        y = int(np.round(40 - (3 + r) * np.cos(degree / 180 * np.pi)))
        print("X:%s,Y:%s  " % (x, y),end="")
        data_R = data[y][x][0]
        data_G = data[y][x][1]
        data_B = data[y][x][2]


        Red[degree][r] = data_R
        Green[degree][r] = data_G
        Blue[degree][r] = data_B
    print()

# print(Red)
# print(Green)
# print(Blue)

print_as_c_array("PIC1_Red", Red)
print_as_c_array("PIC1_Green",Green)
print_as_c_array("PIC1_Blue",Blue)
>>>>>>> 99411e0d0fe281464589e79d9fb00f8538c551b7

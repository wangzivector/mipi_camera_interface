from PIL import Image
import time

frame_width = 1280
frame_height = 800
# frame_width = 640
# frame_height = 400

images_file = open('./build/images_0v9281.raw','rb')
image_num = 0
while(True):

    buffer = images_file.read(frame_width * frame_height)
    if len(buffer) < frame_width * frame_height:
        break

    img = Image.frombuffer('L', ( frame_width, frame_height), buffer)
    img.save('./build/image/from_raw{:0>3d}.bmp'.format(image_num))
    print('./build/image/from_raw{:0>3d}.bmp'.format(image_num))
    image_num += 1
    # img.show()
    # time.sleep(0.02)
    # img.close()


images_file.close()



#!/usr/bin/python3

import scaleapi
import requests
from bs4 import BeautifulSoup

global client
test_mode = True

def test_polygonannotation(client, image_url):
    global test_mode

    if test_mode == True:
        name = 'test test'
    else:
        name = 'Free Space Polygon'

    task = client.create_polygonannotation_task(
        project=name,
        callback_url='http://www.example.com/callback',
        instruction='Draw a polygon around the driveable free space. For detailed\
        instructions:  https://docs.google.com/document/d/e/2PACX-1vTt5wpUu3fr7PRScWvKD96HiQLmatRtqWLPX2IDYZ7yqi8tlMGXI891Q4MQZDXAXgbXX3wW_3APoF1N/pub',
        attachment_type='image',
        attachment=image_url,
        objects_to_annotate=['free space'],
        with_labels=True)
    print (task)

def get_file_list(url, ext=''):
    page = requests.get(url).text
    # print (page)
    soup = BeautifulSoup(page, 'html.parser')
    return [url + '/' + node.get('href') for node in soup.find_all('a') if node.get('href').endswith(ext)]

def main():
    global test_mode
    #client = scaleapi.ScaleClient("test_4ee460b905874ea9a1f76a9c16685e54")
    client = scaleapi.ScaleClient("live_04f964e11f3242d0acc3dfabab3fba8d")

    if test_mode == True:
        print ("test")
    else:
        print ("---------Live mode------------")

    #test_polygonannotation_ok()

    url = 'http://www.csc.calpoly.edu/~jseng/scale_ai_image/test'
    ext = 'jpg'

    imagefile_list = get_file_list(url, ext)

    imagefile_list = imagefile_list[0:10]
    print (len(imagefile_list))
    for file in imagefile_list:
        test_polygonannotation(client, file)

main()

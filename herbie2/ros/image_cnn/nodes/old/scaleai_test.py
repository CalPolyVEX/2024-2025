#!/usr/bin/python3

import scaleapi
import requests
from bs4 import BeautifulSoup

def test_polygonannotation_ok():
    task = client.create_polygonannotation_task(
        callback_url='http://www.example.com/callback',
        instruction='Draw a polygon around the driveable free space. For detailed\
        instructions:  https://docs.google.com/document/d/e/2PACX-1vTt5wpUu3fr7PRScWvKD96HiQLmatRtqWLPX2IDYZ7yqi8tlMGXI891Q4MQZDXAXgbXX3wW_3APoF1N/pub',
        attachment_type='image',
        attachment='http://i.imgur.com/v4cBreD.jpg',
        objects_to_annotate=['free space'],
        with_labels=True)
    print (task)

def listFD(url, ext=''):
    page = requests.get(url).text
    print (page)
    soup = BeautifulSoup(page, 'html.parser')
    return [url + '/' + node.get('href') for node in soup.find_all('a') if node.get('href').endswith(ext)]

def main():
    client = scaleapi.ScaleClient("test_4ee460b905874ea9a1f76a9c16685e54")
    print ("test")

    #test_polygonannotation_ok()

    url = 'http://www.csc.calpoly.edu/~jseng/scale_ai_image/test'
    ext = 'jpg'

    imagefile_list = listFD(url, ext)

    for file in imagefile_list:
        print (file)

    imagefile_list = imagefile_list[0:20]
    print (len(imagefile_list))

main()

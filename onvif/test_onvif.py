from onvif import ONVIFCamera

mycam = ONVIFCamera('10.0.0.53', 80, 'admin', 'a;sldkfjgh3', '/home/jseng/python-onvif/wsdl')

media = mycam.create_media_service()

for p in media.GetVideoEncoderConfigurations():
   print p._token

allProfiles = media.GetProfiles()
mainProfile = media.GetProfile({'ProfileToken' : allProfiles[0]._token})

snapshot = media.GetSnapshotUri({'ProfileToken' : mainProfile._token})

print "test: " + str(snapshot)



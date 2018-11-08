#!/usr/bin/python
import os

#initial checkout 
str1 = 'git clone https://github.com/rpfly3/ORB_SLAM2.git' 
os.system(str1)

#add tf broadcaster
str1 = 'cd ORB_SLAM2; git remote add floatlazer git://github.com/floatlazer/ORB_SLAM2.git'
os.system(str1)
str1 = 'cd ORB_SLAM2; git fetch floatlazer'
os.system(str1)
str1 = 'cd ORB_SLAM2; git cherry-pick 116e27e1c47bb4f20abb69a3b222208c54609ec3'
os.system(str1)
str1 = 'cd ORB_SLAM2; git cherry-pick 5cb4dfc5fac8a890181cae7a4bc718cb919fdf53'
os.system(str1)

#fix compiling error <unistd.h>
#str1 = 'git remote add chenguang055 git://github.com/chenguang055/ORB_SLAM2.git'
#os.system(str1)
#str1 = 'git fetch chenguang055'
#os.system(str1)

#fix DistributOctTree
str1 = 'cd ORB_SLAM2; git remote add OtacilioNeto git://github.com/OtacilioNeto/ORB_SLAM2.git'
os.system(str1)
str1 = 'cd ORB_SLAM2; git fetch OtacilioNeto'
os.system(str1)
str1 = 'cd ORB_SLAM2; git cherry-pick 1ebacf8a42a6d5f0fd3cdd3a02f6dec8bf5585dc'
os.system(str1)

#corrected typedef
str1 = 'cd ORB_SLAM2; git remote add craymichael git://github.com/craymichael/ORB_SLAM2.git'
os.system(str1)
str1 = 'cd ORB_SLAM2; git fetch craymichael'
os.system(str1)
str1 = 'cd ORB_SLAM2; git cherry-pick d5c04468ce85d600f8a0a23fa280b0153fe115e0'
os.system(str1)

#fix multiple assignment of features
str1 = 'cd ORB_SLAM2; git remote add d-vo git://github.com/d-vo/ORB_SLAM2.git'
os.system(str1)
str1 = 'cd ORB_SLAM2; git fetch d-vo'
os.system(str1)
str1 = 'cd ORB_SLAM2; git cherry-pick a49e226446715bba8406911078aedb43226d55d9'
os.system(str1)

#RANSAC subset fix
#str1 = 'cd ORB_SLAM2; git remote add nlw0 git://github.com/nlw0/ORB_SLAM2.git'
#os.system(str1)
#str1 = 'cd ORB_SLAM2; git fetch nlw0'
#os.system(str1)
#str1 = 'cd ORB_SLAM2; git cherry-pick d5c661cd3724b341d9188a46c7c53883be35a046'
#os.system(str1)

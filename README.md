# ue4

Sparse checkout for ROS

```mkdir myrepo
cd catkin_ws/src
git init
git config core.sparseCheckout true
echo 'ros/' > .git/info/sparse-checkout
git remote add -f origin https://github.com/jsseng/ue4.git
git pull origin master
```

Compile roboclaw_driver.py

```
python -m py_compile roboclaw_driver.py
```

This directory contains the training scripts for the neural network.

## Setup
It is best to first setup a Python virtual environment before running PyTorch.  A virtual environment will download all the Python 
packages into a local environment directory and prevent those packages from affecting your local Python installation.  Use the following 
command to setup your virtual environment (you will need to do this only once, and it will create the directory ``pt-env``):

```
python3 -m venv pt-env
```

Once your environment is setup, you will need to activate it each time you want to use that local Python installation:

```
source pt-env/bin/activate
```

Then, install the necessary packages:

```
pip3 install torch torchsummary torchvision albumentations timm
```

---

### Training the model

```python train_ref.py``` - train the Efficientnet-Lite backbone

```python train_ref.py 1``` - train the ground boundary network (has its own separate Efficientnet-Lite backbone)

```python train_ref.py 2``` - train the localization output head

```python train_ref.py 3``` - train the goal prediction output head

```python train_ref.py 4``` - train the turn detection output head




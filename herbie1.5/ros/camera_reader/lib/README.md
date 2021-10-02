
[//]: #
[//]: # "v4l2_helper - README.md file for library"
[//]: #
[//]: # "Copyright (c) 2017-2018, e-con Systems India Pvt. Ltd.  All rights reserved."
[//]: #

## V4L2 Helper

A light-weight V4L2 helper library which abstracts away the
process of fetching frames from a device that supports V4L2.
This library exposes some APIs which could be used to:

 * initialise the V4L2 device
 * read a frame from the V4L2 device
 * de-initialise the V4L2 device

#### Building
To build and install the library separately

```
mkdir build && cd build
cmake ..
make
sudo make install
```

#### Disclaimer
1. This library is provided for the ease of use for developers.
   Anyone with a github account is welcome to contribute to it.
   Please check the [CONTRIBUTING.md](../CONTRIBUTING.md) file to know more.
   
2. This is not a replacement to lib-v4l

#### TODO

1. The library maintains some global state across different
   functions. So, it is not suitable for accessing multiple
   cameras simultaneously.

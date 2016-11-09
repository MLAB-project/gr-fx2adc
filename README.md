gr-fx2adc
==========

Derived from [fx2pipe](http://www.triplespark.net/elec/periph/USB-FX2/software/fx2pipe.html), so that puts us into GPL land.

Temporary instructions
----------------------

To build:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

To run: (in the root directory of the repo)

    # rmmod usbtest # on my box, fx2 is by default claimed by this kernel module
    # LD_LIBRARY_PATH=build/lib PYTHONPATH=build/swig apps/fx2adc_zmq_server


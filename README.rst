layerExRect
===========

This Kirikiri (2/Z) plugin provides a modular replacement for
``Layer.fillRect``, ``Layer.colorRect``, ``Layer.copyRect``, and
``Layer.operateRect``.

Building
--------

After cloning submodules and placing ``ncbind`` and ``tp_stub`` in the
parent directory, a simple ``make`` will generate ``layerExRect.dll``.

How to use
----------

After ``Plugins.link("layerExRect.dll");`` is used, the additional
functions will be exposed under the ``Layer`` class.

License
-------

This project is licensed under the MIT license. Please read the
``LICENSE`` file for more information.

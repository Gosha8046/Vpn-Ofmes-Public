# ui/

Vpn-Ofmes builds its interface entirely in C++ (see `src/ui/` and
`include/vpnofmes/ui/`) rather than from Qt Designer `.ui` files, so that
layout, styling hooks (`setObjectName`) and behaviour stay in one place per
screen and are easy to review as plain code.

This folder is kept as the conventional place to drop `.ui` files if a
screen is ever migrated to Qt Designer; `CMakeLists.txt` already enables
`CMAKE_AUTOUIC`, so any `.ui` file added here will be picked up
automatically.

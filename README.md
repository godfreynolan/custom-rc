This repo contains code, PCB files, and 3D design files for the custom RC prototype.

- `code/gamepad/gamepad.ino` is the Arduino code for reading joystick/button inputs and sending them as USB HID device (e.g. gamepad, mouse) inputs to the main device.
- `pcbs/` contains Kicad design files for the left and right custom PCBs
  - `pcbs/left/` is the left PCB
    - `pcbs/left/outputs.zip` contains Gerber and drill files for ordering the left PCB
  - `pcbs/right/` is the right PCB
    - `pcbs/right/outputs.zip` contains Gerber and drill files for ordering the right PCB
  - Libraries are provided for the joystick and Arduino Pro Micro schematic/layout
- `3d/RC.3mf` is the 3D design file for the 3D printable housing
  - The design is split into many pieces that should be printed separately. All prints can fit on a 22cm x 22cm build plate
  - All parts should be printed facing down (as in, if all parts were being printed together, the whole devie would be oriented with the screen on the build plate)
  - The source project can be found at https://cad.onshape.com/documents/00098938a704a2da40e66fbd/w/5194b559f3115a3992b58aa5/e/1a9a5ca69a7348197c5309db

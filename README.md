## README ##

Mail Box Alert Project, demonstrating basic event driven software concepts while simultaneously creating a nice "maker" project to help my parents!

## Hardware ##

The target hardware for this project may be acquired here:

http://modtronix.com/devkit-sx12i.html


## Build/Project Setup ##

The repo itself expects to be treated as a workspace in Eclipse. Specifically this project was created, built, and tested using the Eclipse based system: "System Workbench for STM32", available here: http://www.openstm32.org/System+Workbench+for+STM32.

Simply import the projects found in this repo into a new workspace.

There are three folders, two builds/projects.
* mailbox_base_station
  * build/project for the binary installed in the "base station" or "main module" portion of modtronix dev kit.
* mailbox_sensor
  * The build/project for the binary installed in the "sensor" or "node" portion of the modtronix dev kit.
* mailbox_common
  * Common code used by both projects/binaries.

To fully understand how to build, download, test, and debug this system, please read and understand the references below, especially the modtronix wiki pages, where the origins of this demo may be found.

To fully duplicate this mailbox project, the "node" or "sensor" that is installed in the mailbox itself, requires minor modifications and attachments. Please contact the author (matthew@covemountainsoftware.com) for details.


## References ##

* Modtronix wiki page for the target board: 
  * http://wiki.modtronix.com/doku.php?id=products:nz-stm32:nz32-sc151

* Schematics of the reference design board: 
  * http://modtronix.com/prod/nz32/nz32-sc151/nz32-sc151-r1.pdf

* Microcontroller:
  * http://www.st.com/web/catalog/mmc/FM141/SC1544/SS1374/LN1041/PF251642?sc=internet/mcu/product/251642.jsp

* modtronix code for the boards:
  * https://github.com/modtronix-com/devkit_sx1276_coide

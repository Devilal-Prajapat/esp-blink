# esp-blink
blink and mqtt example with esp using hivemq, aws
# installation steps
```
getting started:
	https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
	
for windoows installer:
	https://dl.espressif.com/dl/esp-idf/?idf=4.4
		
for vscode plugin: 
	https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md
	
for adding Arduino ide support add this in Preference/setting/additional board manager 
	https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
## add esp-aws-iot sdk 
```
## goto  your project dir
Please clone this branch of the repository using:
	git clone -b "<name_of_the_release_branch>" --recursive https://github.com/espressif/esp-aws-iot
	
For example: To clone just release/release/v3.1.x, you may run:
	git clone -b "release/v3.1.x" --recursive https://github.com/espressif/esp-aws-iot

To add esp-aws-iot sdk to project, update CMakeList.txt 
	set(EXTRA_COMPONENT_DIRS "esp-aws-iot")
```
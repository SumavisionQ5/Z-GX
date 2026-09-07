#!/bin/bash
# Copia el .dol compilado a los DOS lugares necesarios
cp seta-gx.dol /e/apps/ZGX/boot.dol && echo "OK -> sd:/apps/ZGX/boot.dol"
cp seta-gx.dol "/e/wiiflow/plugins/Sega Saturn/ZGX.dol" && echo "OK -> sd:/wiiflow/plugins/Sega Saturn/ZGX.dol"

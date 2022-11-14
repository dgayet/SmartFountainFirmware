<div align="center">

# 86.65 Sistemas Embebidos

## Repositorio para el Firmware (ESP32 Source code) del trabajo final de la materia: Smart Motion Activated Water Fountain

Video explicativo del proyecto: https://youtu.be/tSH6keEnLOM

</div>

Para este proyecto usé una placa NodeMCU ESP32 que tiene integrado un módulo WiFi, necesario para realizar la comunicación con la [aplicación móvil](https://github.com/dgayet/SmartFountainApp).

El dispositivo es un bebedero que tiene incorporado una bomba de agua, la cual se puede activar mediante proximidad, o controlar manualmente mediante la App. También se estima el nivel de agua para determinar si es suficiente para el correcto funcionamiento de la bomba. Existen tres casos:
Nivel de agua LOW: se bloquea la bomba. En la app se muestra que el bebedero está bloqueado y no se puede interactuar con los botones.
Nivel de agua OK: se indica en la app y se permite tanto el funcionamiento manual como el automático.
Nivel de agua MAX: se indica en la app y se bloquea el funcionamiento.
El control manual mediante la aplicación permite activar y desactivar la bomba, y bloquear el bebedero para que no pueda ponerse en funcionamiento.

La comunicación se realiza mediante MQTT, con un broker hosteado en mi PC. 

Para conectar a la placa a la red WiFi se utiliza la aplicación Esptouch. El código fuente es abierto, con lo cual en un futuro se podría agregar a la aplicación que desarrollé para unificarlo. Luego se suscribe de forma automática a los topics del broker necesarios.

Para conectar la app al broker se debe ingresar el Id del bebedero (el cual está guardado en la placa). Éste funciona como topic raíz.

Tuve que realizar un cambio con respecto a la planificación inicial, debido a que no dimensioné correctamente el recipiente del bebedero (fue impreso 3D). En un principio se suponía que la medición del nivel de agua se realizaría mediante un sensor ultrasónico, pero no cabía en el recipiente. Para solucionar esto, se utilizó una celda de carga y, mediante la diferencia de peso se estima el estado del nivel de agua (LOW - OK - MAX). 

## [Memorias del trabajo](https://docs.google.com/document/d/1se82ddqt4nXomL6jscsCLvCwCPkAm0AU/edit?usp=sharing&ouid=113286055275724020143&rtpof=true&sd=true)

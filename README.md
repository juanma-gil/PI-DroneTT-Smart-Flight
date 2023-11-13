# Cómo crear un proyecto para la ESP32 de los Drone TT

## Requisitos
1. Tener instalado vscode.
2. Tener cargada la extensión de c/C++ en vscode.
3. Tener instalado el compilador de c/C++, [c/C++ for Visual Studio Code.](https://code.visualstudio.com/docs/languages/cpp) 
4. Instalar la extensión de [PlatformIO](https://platformio.org/install/ide?install=vscode) en vscode.
5. Ir a `C:\Users\user_name\.platformio\platforms\espressif32\` renombrar el archivo *esp32dev.json* por *esp32dev_old.json*.
6. Cortar y pegar el archivo *esp32dev.json* de este repositorio en la carpeta `C:\Users\user_name\.platformio\platforms\espressif32\`.
7. Agregar el include de la biblioteca del drone tt en `.vscode/c_cpp_properties.json`, la library está en `/lib/RMTT_Libs`.

--

## Api Rest Doc
### GET
#### /battery
```shell
curl IP_DRON/battery --verbose
```
#### /motortime
```shell
curl IP_DRON/battery --verbose
```
#### /speed
```shell
curl IP_DRON/speed --verbose
```
### POST
#### /led
```shell
curl -X POST "IP_DRON/led?r=0&g=0&b=255" --verbose
```
#### /takeoff
```shell
curl -X POST "IP_DRON/takeoff" --verbose
```
#### /path
```shell
curl -X POST "IP_DRON/takeoff" -d '{
    "unit": "cm",
    "points": [
        {
            "x": 100,
            "y": 0,
            "z": 200
        },
        {
            "x": 100,
            "y": 100,
            "z": 200
        },
        {
            "x": 0,
            "y": -200,
            "z": 200
        },
        {
            "x": 200,
            "y": 0,
            "z": 200
        }
    ]
}' --verbose
```
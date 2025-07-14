# Monitor Status With MQTT

Este programa monitora o status de um pino, que possa ser definido como entrada, da placa BitDogLab, e 
envia, a cada 1 segundo, o status atual para o servidor HiveMQ, utilizando 
o protocolo MQTT. 

# Funcionalidades
Leitura do pino e envio do status

# Configuração do WiFi
Para configurar as credenciais do WiFi, edite o arquivo. Localize as seguintes linhas no início do arquivo:

# WiFi credentials
#define WIFI_SSID "WIFI" #define WIFI_PASSWORD "SENHA-DO-WIFI"

Substitua:

Nome da Rede pelo nome da sua rede WiFi (Ex.: "Multilaser 2.4") Senha da Rede pela senha da sua rede WiFi (Ex.: "12345678@")

## USO
Como Usar Compile e envie o executável (.uf2) para o Pico W / BitDogLab Caso seja apenas o Pico W, será necessário conectar os periféricos usados 

Acesse  HiveMQ WebSocket Client https://www.hivemq.com/demos/websocket-client/ 
Conectar-se ao broker.hivemq.com.
Assinar o tópico bitdoglab/status

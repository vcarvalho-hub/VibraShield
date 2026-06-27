# Documentação dos Códigos

## ReadMe.adoc

Arquivo de documentação padrão gerado automaticamente pelo ambiente Arduino. Contém instruções básicas sobre instalação, montagem do circuito, estrutura do projeto e licença. Não interfere no funcionamento do sistema, servindo apenas como documentação.

---

## Sketch.json

Arquivo de configuração do projeto responsável por definir o ambiente de desenvolvimento. Especifica a placa utilizada (ESP32), o tipo de comunicação (Serial) e reserva espaço para bibliotecas e credenciais do projeto.

---

## Sistema de Monitoramento.ino

Arquivo principal do sistema, responsável por toda a lógica de funcionamento do projeto. Realiza a leitura do sensor de som e do sensor piezoelétrico, processa os dados coletados, calibra os sensores, estima os níveis de som e vibração, aciona os LEDs de alerta quando os limites de segurança são ultrapassados e envia as informações para a Arduino IoT Cloud e para o banco de dados Supabase.

### Principais funções

* **ler_pico_a_pico()** – Realiza a leitura dos sensores e calcula a amplitude do sinal.
* **calibrar_som()** – Define o nível de referência do ambiente para o sensor de som.
* **calibrar_piezo()** – Calibra o sensor piezoelétrico em condição de repouso.
* **calcular_som_estimado()** – Converte as leituras em uma estimativa do nível sonoro (dB).
* **calcular_vibracao_mm_s()** – Calcula a velocidade de vibração em mm/s.
* **atualizar_sensores()** – Atualiza as medições, verifica os limites de segurança, aciona os LEDs e gera mensagens de alerta.
* **enviarDadosSupabase()** – Envia periodicamente os dados coletados para o banco de dados Supabase.
* **setup()** – Inicializa o ESP32, configura os pinos, estabelece a comunicação com a Arduino IoT Cloud e realiza a calibração inicial.
* **loop()** – Executa continuamente a leitura dos sensores, atualiza o monitoramento e envia os dados para a nuvem.
* **onSistemaAtivoNuvemChange()** – Atualiza o estado do sistema quando o monitoramento é ativado ou desativado pela plataforma IoT.
* **onComandoPainelChange()** – Callback reservado para futuras funcionalidades de comunicação com o painel.

---

## thingProperties.h

Arquivo gerado automaticamente pela Arduino IoT Cloud. Define as propriedades sincronizadas entre o ESP32 e a plataforma em nuvem, incluindo as variáveis de monitoramento, comandos do painel e configuração da conexão Wi-Fi.

### Principais funções

* **initProperties()** – Inicializa as propriedades e registra as variáveis utilizadas pela Arduino IoT Cloud.
* **WiFiConnectionHandler** – Gerencia a conexão do ESP32 com a rede Wi-Fi e com a plataforma Arduino IoT Cloud.

---

## Resumo Geral

O projeto utiliza um ESP32 para monitorar níveis de som e vibração por meio de sensores dedicados. Os dados são processados localmente, comparados com limites de segurança previamente definidos e utilizados para acionar indicadores visuais (LEDs). Além disso, as informações são enviadas para a Arduino IoT Cloud e armazenadas no banco de dados Supabase, permitindo o monitoramento remoto e a análise em tempo real do sistema.

// 1º: Bibliotecas de Internet e Matemáticas (Antes de tudo!)
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

// 2º: Bibliotecas da Nuvem e Configurações
#include "arduino_secrets.h"
#include "thingProperties.h"

// ================= Configurações Supabase =================
const String SUPABASE_URL = "https://aigwagqryjnwillgrpmq.supabase.co/rest/v1/leituras";
const String SUPABASE_KEY = "sb_publishable_tt6BecPKofeD6BXJYEcRwQ_N4MV2rls";

// ================= Pinos e Configurações =================
const int PINO_SOM = 34;       
const int PINO_PIEZO = 35;     
const int PINO_LED_SOM = 26;   
const int PINO_LED_VIB = 27;    

// ================= Limites de Segurança =================
const float LIMITE_SOM = 70.0;           
const float LIMITE_VIBRACAO_MM_S = 1.0;  

// ================= Ajustes de Amostragem =================
const unsigned long JANELA_AMOSTRAGEM = 50;
const unsigned long INTERVALO_ATUALIZACAO = 250;
const float FATOR_MM_S = 0.01;

// ================= Variáveis Internas e de Estado =================
float referencia_som = 1.0;
float referencia_piezo = 1.0;
float som_filtrado = 0.0;
float vibracao_filtrada = 0.0;
unsigned long ultimo_tempo = 0;

// Transformamos os alarmes em variáveis 100% locais (não vão para a nuvem)
bool alarme_som = false;
bool alarme_vibracao = false;
bool ultimo_alarme_som = false;
bool ultimo_alarme_vibracao = false;

// ================= Funções de Leitura e Cálculo =================
float ler_pico_a_pico(int pino, unsigned long janela_ms) {
  unsigned long inicio = millis();
  int valor_minimo = 4095;
  int valor_maximo = 0;

  while (millis() - inicio < janela_ms) {
    int leitura = analogRead(pino);
    if (leitura < valor_minimo) valor_minimo = leitura;
    if (leitura > valor_maximo) valor_maximo = leitura;
  }
  return (float)(valor_maximo - valor_minimo);
}

void calibrar_som() {
  const int quantidade_amostras = 60;
  float soma = 0;
  Serial.println("Calibrando som... deixe o ambiente silencioso.");
  for (int i = 0; i < quantidade_amostras; i++) {
    soma += ler_pico_a_pico(PINO_SOM, JANELA_AMOSTRAGEM);
    delay(10);
  }
  referencia_som = soma / quantidade_amostras;
  if (referencia_som < 1.0) referencia_som = 1.0;

  Serial.print("Nivel de silencio do som: ");
  Serial.println(referencia_som);
}

void calibrar_piezo() {
  const int quantidade_amostras = 60;
  float soma = 0;
  Serial.println("Calibrando vibracao... deixe a mesa parada.");
  for (int i = 0; i < quantidade_amostras; i++) {
    soma += ler_pico_a_pico(PINO_PIEZO, JANELA_AMOSTRAGEM);
    delay(10);
  }
  referencia_piezo = soma / quantidade_amostras;
  if (referencia_piezo < 1.0) referencia_piezo = 1.0;
}

float calcular_som_estimado(float pico_a_pico) {
  float razao = (pico_a_pico + 1.0) / (referencia_som + 1.0);
  float valor = 40 + 20.0 * log10(razao);
  if (valor < 0) valor = 0;
  return valor;
}

float calcular_vibracao_mm_s(float pico_a_pico) {
  float leitura_util = pico_a_pico - referencia_piezo;
  if (leitura_util < 0) leitura_util = 0;
  return leitura_util * FATOR_MM_S;
}

// ================= Atualização Principal =================
void atualizar_sensores() {
  float pico_som = ler_pico_a_pico(PINO_SOM, JANELA_AMOSTRAGEM);
  float pico_piezo = ler_pico_a_pico(PINO_PIEZO, JANELA_AMOSTRAGEM);

  float som_atual = calcular_som_estimado(pico_som);
  float vibracao_atual = calcular_vibracao_mm_s(pico_piezo);

  som_filtrado = 0.8 * som_filtrado + 0.2 * som_atual;
  vibracao_filtrada = 0.8 * vibracao_filtrada + 0.2 * vibracao_atual;

  som_db_estimado = som_filtrado;
  vibracao_mm_s = vibracao_filtrada;

  alarme_som = (som_db_estimado >= LIMITE_SOM);
  alarme_vibracao = (vibracao_mm_s >= LIMITE_VIBRACAO_MM_S);

  digitalWrite(PINO_LED_SOM, alarme_som ? HIGH : LOW);
  digitalWrite(PINO_LED_VIB, alarme_vibracao ? HIGH : LOW);

  // ================= Lógica de Disparo (Painel da Nuvem) =================
  
  // ALERTA DE SOM
  if (alarme_som && !ultimo_alarme_som) {
    comando_painel = "ALERTA: Nivel de som critico! (" + String(som_db_estimado, 1) + " dB)";
  } else if (!alarme_som && ultimo_alarme_som) {
    comando_painel = "STATUS: Nivel de som normalizado.";
  }

  // ALERTA DE VIBRAÇÃO
  if (alarme_vibracao && !ultimo_alarme_vibracao) {
    comando_painel = "ALERTA: Vibracao critica! (" + String(vibracao_mm_s, 2) + " mm/s)";
  } else if (!alarme_vibracao && ultimo_alarme_vibracao) {
    comando_painel = "STATUS: Vibracao normalizada.";
  }

  ultimo_alarme_som = alarme_som;
  ultimo_alarme_vibracao = alarme_vibracao;
}

// ================= Função de Envio para Supabase =================
void enviarDadosSupabase() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SUPABASE_URL);
    http.setTimeout(3000); 
    
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", "Bearer " + SUPABASE_KEY);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");

    String jsonPayload = "{\"som\":" + String(som_db_estimado, 2) + 
                         ",\"vibracao\":" + String(vibracao_mm_s, 2) + "}";

    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      Serial.print("✅ Supabase atualizado. Código: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("❌ Erro Supabase: ");
      Serial.println(http.errorToString(httpResponseCode));
    }
    http.end();
  }
}

// =====================
// SETUP
// =====================
void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(PINO_LED_SOM, OUTPUT);
  pinMode(PINO_LED_VIB, OUTPUT);
  digitalWrite(PINO_LED_SOM, LOW);
  digitalWrite(PINO_LED_VIB, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PINO_SOM, ADC_11db);
  analogSetPinAttenuation(PINO_PIEZO, ADC_11db);

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  calibrar_som();
  calibrar_piezo();
  
  sistema_ativo_nuvem = true;
}

// =====================
// LOOP PRINCIPAL
// =====================
void loop() {
  ArduinoCloud.update();

  if (!sistema_ativo_nuvem) {
    digitalWrite(PINO_LED_SOM, LOW);
    digitalWrite(PINO_LED_VIB, LOW);
    som_db_estimado = 0;
    vibracao_mm_s = 0;
    alarme_som = false;
    alarme_vibracao = false;
    return;
  }

  if (millis() - ultimo_tempo >= INTERVALO_ATUALIZACAO) {
    atualizar_sensores();
    ultimo_tempo = millis();

  }

  static unsigned long lastSupabase = 0;
  if (millis() - lastSupabase > 2000) {
    enviarDadosSupabase();
    lastSupabase = millis();
  }
}

// ================= Callbacks da IoT Cloud =================
void onSistemaAtivoNuvemChange() {
  comando_painel = sistema_ativo_nuvem ? "CONFIRMADO: Monitoramento ATIVADO" : "CONFIRMADO: Monitoramento PAUSADO";
}

void onComandoPainelChange() {
}

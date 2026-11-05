const int NUM_GAVETAS = 6;

// Pinos dos LEDs (Gavetas 1 a 6)
const int pinosLeds[NUM_GAVETAS] = {25, 26, 27, 14, 12, 33};

// Pinos dos Sensores Fim de Curso (Gavetas 1 a 6)
const int pinosSensores[NUM_GAVETAS] = {13, 16, 17, 18, 19, 21};

// Pino do Buzzer
const int pinoBuzzer = 22;

#define GAVETA_FECHADA LOW
#define GAVETA_ABERTA  HIGH

enum EstadoAlarme {
  AGUARDANDO,       // Todos os LEDs apagados, esperando digitar no Serial
  TOCANDO_ALARME,   // LED da gaveta aceso fixo e Buzzer apitando
  GAVETA_ABERTA_OK  // Gaveta certa foi aberta, Buzzer para e LED pisca esperando fechar
};

EstadoAlarme estadoAtual = AGUARDANDO;
int gavetaAlvo = -1;

unsigned long ultimoBip = 0;
bool estadoBip = false;
bool estadoAnterioresGavetas[NUM_GAVETAS];

void setup() {
  Serial.begin(115200);
  pinMode(pinoBuzzer, OUTPUT);

  // Inicializa todos os LEDs apagados e lê a posição inicial das chaves
  for (int i = 0; i < NUM_GAVETAS; i++) {
    pinMode(pinosLeds[i], OUTPUT);
    digitalWrite(pinosLeds[i], LOW);
    
    pinMode(pinosSensores[i], INPUT_PULLUP);
    estadoAnterioresGavetas[i] = digitalRead(pinosSensores[i]);
  }

  Serial.println("================================================");
  Serial.println("  GAVETEIRO INTELIGENTE (6 GAVETAS) PRONTO!");
  Serial.println("================================================");
  Serial.println("Digite de '1' a '6' no Monitor Serial e de Enter");
  Serial.println("para acender o LED e apitar o Buzzer da gaveta.");
  Serial.println("------------------------------------------------");
}

void loop() {
  lerComandosSerial();
  monitorarGavetas();
  gerenciarAlertas();
  delay(20);
}

void lerComandosSerial() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd >= '1' && cmd <= '6') {
      int indice = cmd - '1';
      dispararHorarioRemedio(indice);
    }
  }
}

void dispararHorarioRemedio(int gaveta) {
  gavetaAlvo = gaveta;
  estadoAtual = TOCANDO_ALARME;

  // 1. Acende imediatamente o LED da gaveta escolhida (e apaga os outros)
  for (int i = 0; i < NUM_GAVETAS; i++) {
    digitalWrite(pinosLeds[i], (i == gavetaAlvo) ? HIGH : LOW);
  }

  // 2. Começa a apitar o Buzzer no exato momento em que o LED acende
  ultimoBip = millis();
  estadoBip = true;
  tone(pinoBuzzer, 1000);

  Serial.printf("\n>>> [ALARME] Hora do remedio da GAVETA %d! (LED %d ACESO + BUZZER) <<<\n", gavetaAlvo + 1, gavetaAlvo + 1);
  Serial.printf("Abra a chave %d para pegar o remedio.\n", gavetaAlvo + 1);
}

void monitorarGavetas() {
  for (int i = 0; i < NUM_GAVETAS; i++) {
    bool leituraAtual = digitalRead(pinosSensores[i]);

    if (leituraAtual != estadoAnterioresGavetas[i]) {
      estadoAnterioresGavetas[i] = leituraAtual;

      if (leituraAtual == GAVETA_ABERTA) {
        Serial.printf("[SENSOR] Gaveta %d foi ABERTA.\n", i + 1);
        tratarAberturaGaveta(i);
      } else {
        Serial.printf("[SENSOR] Gaveta %d foi FECHADA.\n", i + 1);
        tratarFechamentoGaveta(i);
      }
    }
  }
}

void tratarAberturaGaveta(int gavetaAberta) {
  if (estadoAtual == TOCANDO_ALARME) {
    if (gavetaAberta == gavetaAlvo) {
      // Abriu a gaveta correta: silencia o buzzer e espera fechar
      noTone(pinoBuzzer);
      estadoAtual = GAVETA_ABERTA_OK;
      Serial.printf("[SUCESSO] Gaveta %d correta aberta! Feche a gaveta apos pegar o remedio.\n", gavetaAberta + 1);
    } else {
      // Abriu a gaveta errada durante o alarme
      Serial.printf("[ERRO] Gaveta %d ERRADA! O remedio atual esta na Gaveta %d.\n", gavetaAberta + 1, gavetaAlvo + 1);
      tone(pinoBuzzer, 250, 400); // Som grave de erro
    }
  } else if (estadoAtual == AGUARDANDO) {
    // Abriu uma gaveta sem ter horário programado
    Serial.printf("[AVISO] Gaveta %d aberta fora de horario!\n", gavetaAberta + 1);
    tone(pinoBuzzer, 250, 400);
  }
}

void tratarFechamentoGaveta(int gavetaFechada) {
  if (estadoAtual == GAVETA_ABERTA_OK && gavetaFechada == gavetaAlvo) {
    // Fechou a gaveta correta: apaga o LED e dá um bip curto de conclusão
    digitalWrite(pinosLeds[gavetaAlvo], LOW);
    noTone(pinoBuzzer);
    estadoAtual = AGUARDANDO;
    gavetaAlvo = -1;

    tone(pinoBuzzer, 2000, 150);
    Serial.println("[CONCLUIDO] Gaveta fechada. Digite 1 a 6 para testar outro horario.\n");
  }
}

void gerenciarAlertas() {
  unsigned long agora = millis();

  if (estadoAtual == TOCANDO_ALARME) {
    // Garante que o LED da gaveta alvo permaneça aceso fixo enquanto o buzzer apita
    digitalWrite(pinosLeds[gavetaAlvo], HIGH);

    if (agora - ultimoBip >= 500) {
      ultimoBip = agora;
      estadoBip = !estadoBip;
      if (estadoBip) {
        tone(pinoBuzzer, 1000);
      } else {
        noTone(pinoBuzzer);
      }
    }
  } 
  else if (estadoAtual == GAVETA_ABERTA_OK) {
    // Pisca o LED da gaveta aberta lembrando de fechar
    if (agora - ultimoBip >= 300) {
      ultimoBip = agora;
      estadoBip = !estadoBip;
      digitalWrite(pinosLeds[gavetaAlvo], estadoBip ? HIGH : LOW);
    }
  }
}

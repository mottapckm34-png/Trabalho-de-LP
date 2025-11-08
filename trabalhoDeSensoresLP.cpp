#include <list>
#include <vector>
#include <algorithm>
#include <numeric>


#define SOIL_TEMP_PIN 4  
#define SOIL_HUMIDITY_PIN A0 
#define SALINITY_PIN A1 
#define PINO_RELE_IRRIGACAO 3 


struct DadosSolo {
  float temperaturaSolo;
  float umidadeSolo;
  float salinidade;
  unsigned long tempoColeta;

  String toSerialString() const {
    return String(temperaturaSolo, 1) + ";" + String(umidadeSolo, 1) + ";" + String(salinidade, 2);
  }
};

// Constantes de Tempo e Coleta
const unsigned long INTERVALO_COLETA = 15UL * 60UL * 1000UL; 
const int NUM_COLETAS_CICLO = 16; 

// Limites para Decisão de Irrigação 
const float LIMITE_UMIDADE_MIN = 30.0; 
const float LIMITE_TEMPERATURA_MEDIA_CRITICA = 30.0; 
const float LIMITE_SALINIDADE_MAX = 2.0;

//Variáveis de Controle
std::list<DadosSolo> dadosColetados;
unsigned long ultimaColeta = 0;
int contadorColetas = 0;
float ultimaTemperaturaMediaQuatroHoras = 0.0; 


float lerTemperaturaSolo() {

  return 25.5; 
}

float lerUmidadeSolo() {
  int leituraSoloBruta = analogRead(SOIL_HUMIDITY_PIN);

  return map(leituraSoloBruta, 0, 1023, 0, 100); 
}

float lerSalinidade() {
  int leituraSal = analogRead(SALINITY_PIN);
 
  return ((float)leituraSal / 1023.0) * 5.0;
}

DadosSolo coletarDadosSolo() {
  DadosSolo dados;
  
  dados.temperaturaSolo = lerTemperaturaSolo();
  dados.umidadeSolo = lerUmidadeSolo();
  dados.salinidade = lerSalinidade();
  dados.tempoColeta = millis();

  return dados;
}

// Fucão de decisão de irrigacão
void tomarDecisaoIrrigacao(const DadosSolo &dados) {
  bool deveIrrigar = false;
  String motivo = "Condições normais.";

  // Condição de Umidade Baixa
  if (dados.umidadeSolo < LIMITE_UMIDADE_MIN) {
    deveIrrigar = true;
    motivo = "Umidade do solo baixa (" + String(dados.umidadeSolo, 1) + "%)";
  }
  
  else if (ultimaTemperaturaMediaQuatroHoras > LIMITE_TEMPERATURA_MEDIA_CRITICA) {
    deveIrrigar = true;
    motivo = "Alta demanda hídrica (Temp Solo Média 4h: " + String(ultimaTemperaturaMediaQuatroHoras, 1) + "°C)";
  }

 
  else if (dados.salinidade > LIMITE_SALINIDADE_MAX) {
    if (dados.umidadeSolo < 55.0) {
     
      deveIrrigar = true;
      motivo = "Salinidade alta e solo seco. Irrigar para lixiviação.";
    } else {
  
      motivo = "Salinidade alta (" + String(dados.salinidade, 2) + " dS/m). Monitorar.";
    }
  }

  // Acão de irrigação
  if (deveIrrigar) {
    digitalWrite(PINO_RELE_IRRIGACAO, HIGH);
    Serial.print("DECISÃO: *IRRIGAR* (Motivo: ");
    Serial.print(motivo);
    Serial.println(")");
    
  } else {
    digitalWrite(PINO_RELE_IRRIGACAO, LOW); 
    Serial.print("DECISÃO: Não Irrigar (Motivo: ");
    Serial.print(motivo);
    Serial.println(")");
  }
}

void apresentarColetaNoSerial(int contador, const DadosSolo &dados) {
  Serial.print("[COLETA ");
  if (contador < 10) Serial.print("0");
  Serial.print(contador);
  Serial.print("] Temp: "); Serial.print(dados.temperaturaSolo, 1);
  Serial.print(" | Umid: "); Serial.print(dados.umidadeSolo, 1);
  Serial.print(" | Sal: "); Serial.print(dados.salinidade, 2);
  Serial.print(" | ");
}

void processarEGerarRelatorio(std::list<DadosSolo> &listaDados) {
  if (listaDados.size() < 3) {
    Serial.println("\n[RELATORIO] Poucos dados para cálculo da média.");
    return;
  }

  // Coletar todas as temperaturas do solo
  std::vector<float> temperaturas;
  for (auto &dado : listaDados)
    temperaturas.push_back(dado.temperaturaSolo);

  // Ordenar e identificar extremos
  std::sort(temperaturas.begin(), temperaturas.end());
  float menor = temperaturas.front();
  float maior = temperaturas.back();

  // Remover o menor e o maior
  temperaturas.erase(temperaturas.begin());
  temperaturas.pop_back();

  // Calcular a média dos restantes
  float soma = std::accumulate(temperaturas.begin(), temperaturas.end(), 0.0);
  float media = soma / temperaturas.size();
  
  // Atualiza a variavel global para a proxima decisão de irrigacão
  ultimaTemperaturaMediaQuatroHoras = media; 

  // Gerar o relatório final
  Serial.println("\n");
  Serial.println("RELATORIO_FINAL_4_HORAS.TXT");
  Serial.println("\n");
  Serial.println("Dados_Processados_Temperatura_Splo");
  Serial.print("Data_Hora_Processamento: "); Serial.println(millis());
  Serial.print("Total_Coletas_Iniciais: "); Serial.println(listaDados.size());
  Serial.print("Valor_Menor_Removidos: "); Serial.println(menor, 2);
  Serial.print("Valor_Maior_Removidos: "); Serial.println(maior, 2);
  Serial.print("Media_Temperatura_Solo: "); Serial.println(media, 2);
  Serial.println("\n");
  Serial.println("Dados_ColetadosS (Temp;Umid;Sal)");
  
  // Imprime todos os dados brutos coletados no ciclo
  for (auto &dado : listaDados) {
    Serial.println(dado.toSerialString());
  }
  
  Serial.println("\n");

 
  listaDados.clear();
  contadorColetas = 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(PINO_RELE_IRRIGACAO, OUTPUT);
  digitalWrite(PINO_RELE_IRRIGACAO, LOW); // Garante que o relé está desligado no início
  
  Serial.println("\nColetor de Dados do Solo com Irrigação ");
  Serial.println("\n");
  ultimaColeta = millis(); // Inicializa o tempo de referência
}

void loop() {
  unsigned long tempoAtual = millis();

  // Checa se é hora de coletar dados 
  if (tempoAtual - ultimaColeta >= INTERVALO_COLETA) {
    
    DadosSolo dado = coletarDadosSolo();
    
    // Adiciona a medição à lista
    dadosColetados.push_back(dado);
    contadorColetas++;
    
    apresentarColetaNoSerial(contadorColetas, dado);
    tomarDecisaoIrrigacao(dado);

    
    if (contadorColetas >= NUM_COLETAS_CICLO) {
      processarEGerarRelatorio(dadosColetados);
    }
    
    // Atualiza o tempo de referência para o próximo ciclo
    ultimaColeta = tempoAtual;
  }
}


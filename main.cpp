using namespace std;
#include <iostream>
#include <bitset>
#include <random>
#include <list>
#include <iterator>

#define NOISE_RATE 0.01;

enum edc_type {
  EVEN_PARITY,
  ODD_PARITY,
  CRC
};

/**
 * Struct para representacao de um quadro que eh
 * transmitido pelo meio de comunicacao. Eh dividido
 * entre o datagram, que contem o conteudo bruto da mensagem,
 * e o edc_code, que traz informacoes sobre a checagem de
 * paridades do datagrama, utilizada para checagem de erros.
 * 
 * Datagram: string formada por caracteres 0 ou 1, representando
 * um fluxo bruto de bits.
 * 
 * edc_code: string formada por caracteres 0 ou 1, representando
 * as paridades calculadas. Em ordem, sao colocados os bits de
 * paridades para cada linha, seguidos pelas paridades da coluna.
 */
struct frame {
  string datagram;
  int edc_type;
  string edc_code;
};

void transmission_application();
void transmission_application_layer(string message);
void transmission_link_layer(string bitstream, int edc_type = edc_type::EVEN_PARITY);
void transmission_error_controller(frame *frame);

// Construcao de paridade (camada transmissora)
void transmission_even_parity(frame *frame);
void transmission_odd_parity(frame *frame);

void communication_channel(frame *frame);

void receiver_link_layer(frame *frame);
// Checagem de paridade (camada receptora)
void receiver_even_parity(frame *frame);
void receiver_odd_parity(frame *frame);
void receiver_application_layer(frame *frame);
void receiver_application(string message);

// Funcao auxiliar para mostrar no console a matriz de paridade
void print_parity_matrix(frame *frame);

int main(int argc, char **argv) {
  transmission_application();
  return 0;
}

// =====================================================================
// *************************** IMPLEMENTACAO ***************************
// =====================================================================

void print_parity_matrix(frame *frame) {
  cout << "\n";
  int edc_index = 0;
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    for (int j = 0; j < 8; j++) {
      cout << frame->datagram[i * 8 + j];
    }
    cout << " " << frame->edc_code[i] << endl;
    edc_index++;
  }
  cout << "\n";

  /**
   * Neste ponto, terminei de printar as paridades
   * para as linhas. As paridades restantes presente
   * na cadeia frame->edc_code sao respectivos as
   * colunas.
   */
  for (int i = edc_index; i < frame->edc_code.size(); i++) {
    cout << frame->edc_code[i];
  }
  cout << "\n" << endl;
}

void transmission_application() {
  string message;
  cout << "Digite a mensagem a ser transmitida:" << endl;
  getline(cin, message);
  cout << message.size() << endl;
  transmission_application_layer(message);
}

void transmission_application_layer(string message) {
  string datagram = "";
  for (size_t i = 0; i < message.size(); i++) {
    cout << message[i] << " = " << bitset<8>(message.c_str()[i]) << endl;
    datagram += bitset<8>(message.c_str()[i]).to_string();
  }
  // Paridade par esta definida como default na funcao
  transmission_link_layer(datagram);

  // Eh possivel passar o tipo de paridade explicitamente para outras opcoes
  // transmission_link_layer(datagram, edc_type::ODD_PARITY);
}

void transmission_link_layer(string datagram, int edc_type) {
  frame new_frame;
  new_frame.datagram = datagram;
  new_frame.edc_type = edc_type;

  // Calcular o edc code baseado no edc_type (enquadrar antes de enviar)
  switch (edc_type) {
    case edc_type::EVEN_PARITY:
      transmission_even_parity(&new_frame);
      break;
    case edc_type::ODD_PARITY:
      transmission_odd_parity(&new_frame);
      break;
    default:
      break;
  }
  cout << "====================== TRANSMISSOR =======================" << endl;
  print_parity_matrix(&new_frame);
  cout << "==========================================================\n" << endl;
  communication_channel(&new_frame);
}

void transmission_even_parity(frame *frame) {
  string edc_code = "";
  int even_counter = 0;
  /**
   * Considerando que teremos uma cadeia de bits proveniente
   * de uma mensagem em bytes, com toda certeza teremos
   * tamanhos multiplos de 8.
   * Por definicao, nosso algoritmo construira a matriz de paridade
   * com 8 colunas para simplificar as logicas. 
   */

  // Paridade por linha: cada 8 bits me dara um valor de paridade
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    // Contagem de pares dentro de um trecho sequencial de 8 bits
    for (int j = 0; j < 8; j++) {
      if (frame->datagram[i * 8 + j] == '1') {
        even_counter++;
      }
    }
    // Se obtivemos um numero par de '1s', a paridade eh 0.
    edc_code += even_counter % 2 == 0 ? '0' : '1';
    even_counter = 0;
  }

  /**
   *  Paridade por coluna: se n eh o tamanho da cadeia de bits,
   * teremos um total de n / 8 linhas na matriz, o que corresponde
   * ao comprimento de cada coluna. Temos um total de 8 colunas (fixo)
   * para iterar.
   */
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < frame->datagram.size(); j += 8) {
      if (frame->datagram[i + j] == '1') {
        even_counter++;
      }
    }
    edc_code += even_counter % 2 == 0 ? '0' : '1';
    even_counter = 0;
  }

  /**
   *  Finalizada a construcao do edc, coloco isso
   * como no frame como "metadado" que sera lido pelo
   * receptor para checagem de erros.
   */
  frame->edc_code = edc_code;
}

void transmission_odd_parity(frame *frame) {
  string edc_code = "";
  int even_counter = 0;

  // Paridade por linha: cada 8 bits me dara um valor de paridade
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    // Contagem de pares dentro de um trecho sequencial de 8 bits
    for (int j = 0; j < 8; j++) {
      if (frame->datagram[i * 8 + j] == '1') {
        even_counter++;
      }
    }
    // Se obtivemos um numero par de '1s', a paridade eh 0.
    edc_code += even_counter % 2 == 0 ? '1' : '0';
    even_counter = 0;
  }

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < frame->datagram.size(); j += 8) {
      if (frame->datagram[i + j] == '1') {
        even_counter++;
      }
    }
    edc_code += even_counter % 2 == 0 ? '1' : '0';
    even_counter = 0;
  }
  frame->edc_code = edc_code;
}

void communication_channel(frame *frame) {
  cout << "Transmitindo mensagem..." << endl;
  string noisy_datagram = "";

  /* Geracao de numeros randomicos para "sujar" o
  fluxo de bits com erros. */
  float error_percentage = NOISE_RATE;
  random_device rd;
  mt19937 gen(rd());
  uniform_real_distribution<> dis(0, 1);

  /* Procuro fazer a troca de bits em certas
  posicoes para gerar uma mensagem com erros. */
  for (size_t i = 0; i < frame->datagram.size(); i++) {
    // Bits que sao repassados sem erros
    if (dis(gen) > error_percentage) {
      noisy_datagram += frame->datagram[i];
    } else { // Bits selecionados sao invertidos, indicando erro
      noisy_datagram += frame->datagram[i] == '0' ? '1' : '0';
    }
  }
  
  cout << frame->datagram << " (Mensagem original)" << endl;
  cout << noisy_datagram << " (Mensagem com ruido)" << endl;
  
  frame->datagram = noisy_datagram;
  receiver_link_layer(frame);
}

void receiver_link_layer(frame *frame) {
  cout << "\n===================== RECEPTOR ===========================" << endl;
  print_parity_matrix(frame);
  cout << "==========================================================\n" << endl;
  switch (frame->edc_type) {
    case edc_type::EVEN_PARITY:
      receiver_even_parity(frame);
      break;
    case edc_type::ODD_PARITY:
      receiver_odd_parity(frame);
      break;
    default:
      break;
  }
  receiver_application_layer(frame);
}

void receiver_even_parity(frame *frame) {
  cout << "Verificando erros...\n" << endl; 
  int even_counter = 0;
  int edc_index = 0;
  list<int> rows, columns;

  // Checando paridade das linhas
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (frame->datagram[i * 8 + j] == '1') {
        even_counter++;
      }
    }
    // Batendo o edc passado com a conta feita pelo receptor
    if (frame->edc_code[i] == '1' && even_counter % 2 == 0) {
      cout << "Erro na linha " << i  << endl;
      rows.push_front(i);

    } else if (frame->edc_code[i] == '0' && even_counter % 2 != 0) {
      cout << "Erro na linha " << i << endl;
      rows.push_front(i);
    }
    even_counter = 0;
    edc_index++;
  }

  // Checando paridade das colunas
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < frame->datagram.size(); j += 8) {
      if (frame->datagram[i + j] == '1') {
        even_counter++;
      }
    }
    // Batendo o edc passado com a conta feita pelo receptor
    if (frame->edc_code[edc_index] == '1' && even_counter % 2 == 0) {
      cout << "Erro na coluna " << i << endl;
      columns.push_front(i);

    } else if (frame->edc_code[edc_index] == '0' && even_counter % 2 != 0) {
      cout << "Erro na coluna " << i << endl;
      columns.push_front(i);
    }
    even_counter = 0;
    edc_index++;
  }

  /**
   * Correcao de erros: casos simples em que ha apenas uma coluna
   * e uma linha apresentando erro podem ser corrigidos sem problemas,
   * fazendo a troca de um unico bit. No entanto, outros casos, como
   * combinacoes pares de linhas e colunas com erro (2 linhas, 2 colunas)
   * fazem com que nao possamos garantir exatamente quais interseccao de
   * fato possuem erro. Como ja eh sabido que estes metodos nao sao 100%
   * confiaveis, fazemos uma abordagem de correcao que certamente ira corrigir
   * os casos mais simples, com uma chance menor, porem existente, de corrigir
   * casos mais complexos.
   * Assim, tento fazer correcao de bits linha a linha que apresentou erro: para
   * cada linha, procuro a primeira coluna que a intercepta e que nao recebeu correcao
   * ainda - em caso existente, faco a troca daquele bit.
   */
  list <int> :: iterator it_row, it_col;
  it_col = columns.begin();

  // Para cada linha a ser corrigida...
  for (it_row = rows.begin(); it_row != rows.end(); it_row++) {
    // Se ainda ha colunas para corrigir...
    if (it_col != columns.end()) {
      int index = ( (*it_row) * 8) + (*it_col);
      frame->datagram[index] = frame->datagram[index] == '1' ? '0' : '1';
      it_col++;
    }
  }
}

void receiver_odd_parity(frame *frame) {
  cout << "Verificando erros...\n" << endl; 
  int even_counter = 0;
  int edc_index = 0;
  list<int> rows, columns;

  // Checando paridade das linhas
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (frame->datagram[i * 8 + j] == '1') {
        even_counter++;
      }
    }
    // Batendo o edc passado com a conta feita pelo receptor
    if (frame->edc_code[i] == '1' && even_counter % 2 != 0) {
      cout << "Erro na linha " << i << endl;
      rows.push_front(i);

    } else if (frame->edc_code[i] == '0' && even_counter % 2 == 0) {
      cout << "Erro na linha " << i << endl;
      rows.push_front(i);
    }
    even_counter = 0;
    edc_index++;
  }

  // Checando paridade das colunas
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < frame->datagram.size(); j += 8) {
      if (frame->datagram[i + j] == '1') {
        even_counter++;
      }
    }
    // Batendo o edc passado com a conta feita pelo receptor
    if (frame->edc_code[edc_index] == '1' && even_counter % 2 != 0) {
      cout << "Erro na coluna " << i << endl;
      columns.push_front(i);

    } else if (frame->edc_code[edc_index] == '0' && even_counter % 2 == 0) {
      cout << "Erro na coluna " << i  << endl;
      columns.push_front(i);
    }
    even_counter = 0;
    edc_index++;
  }

  list <int> :: iterator it_row, it_col;
  it_col = columns.begin();

  // Para cada linha a ser corrigida...
  for (it_row = rows.begin(); it_row != rows.end(); it_row++) {
    // Se ainda ha colunas para corrigir...
    if (it_col != columns.end()) {
      int index = ( (*it_row) * 8) + (*it_col);
      frame->datagram[index] = frame->datagram[index] == '1' ? '0' : '1';
      it_col++;
    }
  }
}

void receiver_application_layer(frame *frame) {
  string message = "";
  for (size_t i = 0; (i * 8) < frame->datagram.size(); i++) {
    int decimal = stoi(frame->datagram.substr(i*8, 8), nullptr, 2);
    message += static_cast<char>(decimal);
  }
  receiver_application(message);
}

void receiver_application(string message) {
  cout << "@@@ Aplicacao receptora @@@\n" << endl;
  cout << "Mensagem recebida: " << message << endl;
}
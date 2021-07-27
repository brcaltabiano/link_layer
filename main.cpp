using namespace std;
#include <iostream>
#include <bitset>
#include <random>

enum edc_type {
  EVEN_PARITY,
  ODD_PARITY,
  CRC
};

struct frame {
  string datagram;
  int edc_type;
  string edc_code;
};

void emitter_application();
void application_layer(string message);
void link_layer(string bitstream, int edc_type = edc_type::EVEN_PARITY);
void link_layer_error_controller(frame *frame);
void transmission_channel(frame *frame);

// Construcao de paridade (camada transmissora)
void even_parity_build(frame *frame);
void odd_parity_build(frame *frame);
void crc_build(frame *frame);

// Checagem de paridade (camada receptora)
void even_parity_check(frame *frame);
void odd_parity_check(frame *frame);
void crc_check(frame *frame);

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

int main(int argc, char **argv) {
  emitter_application();
  return 0;
}

void emitter_application() {
  string message = "hello";
  application_layer(message);
}

void application_layer(string message) {
  string datagram = "";
  for (size_t i = 0; i < message.size(); i++) {
    cout << message[i] << " = " << bitset<8>(message.c_str()[i]) << endl;
    datagram += bitset<8>(message.c_str()[i]).to_string();
  }
  link_layer(datagram);
}

void link_layer(string datagram, int edc_type) {
  frame new_frame;
  new_frame.datagram = datagram;
  new_frame.edc_type = edc_type;

  // Calcular o edc code baseado no edc_type (enquadrar antes de enviar)
  switch (edc_type) {
    case edc_type::EVEN_PARITY:
      even_parity_build(&new_frame);
      break;
    case edc_type::ODD_PARITY:
      odd_parity_build(&new_frame);
      break;
    case edc_type::CRC:
      crc_build(&new_frame);
      break;
    default:
      break;
  }
  print_parity_matrix(&new_frame);
  transmission_channel(&new_frame);
}

void even_parity_build(frame *frame) {
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
    // Se obtivemos um numero par de '1s', a paridade eh 1.
    edc_code += even_counter % 2 == 0 ? '1' : '0';
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
    edc_code += even_counter % 2 == 0 ? '1' : '0';
    even_counter = 0;
  }

  /**
   *  Finalizada a construcao do edc, coloco isso
   * como no frame como "metadado" que sera lido pelo
   * receptor para checagem de erros.
   */
  frame->edc_code = edc_code;
}

void odd_parity_build(frame *frame) {

}

void crc_build(frame *frame) {

}

void transmission_channel(frame *frame) {
  string noisy_datagram = "";

  /* Geracao de numeros randomicos para "sujar" o
  fluxo de bits com erros. */
  float error_percentage = 0.2;
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
  
  cout << frame->datagram << " (original message)" << endl;
  cout << noisy_datagram << " (noisy message)" << endl;
  
  frame->datagram = noisy_datagram;
  link_layer_error_controller(frame);
}

void link_layer_error_controller(frame *frame) {
  print_parity_matrix(frame);
  switch (frame->edc_type) {
    case edc_type::EVEN_PARITY:
      even_parity_check(frame);
      break;
    case edc_type::ODD_PARITY:
      odd_parity_check(frame);
      break;
    case edc_type::CRC:
      crc_check(frame);
      break;
    default:
      break;
  }
}

void even_parity_check(frame *frame) {
  int even_counter = 0;
  int edc_index = 0;

  // Checando paridade das linhas
  for (int i = 0; i < frame->datagram.size() / 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (frame->datagram[i * 8 + j] == '1') {
        even_counter++;
      }
    }
    // Batendo o edc passado com a conta feita pelo receptor
    if (frame->edc_code[i] == '1' && even_counter % 2 != 0) {
      cout << "Erro na linha " << i << ": dataframe transmitido possui paridade IMPAR, mas EDC informa paridade PAR." << endl;

    } else if (frame->edc_code[i] == '0' && even_counter % 2 == 0) {
      cout << "Erro na linha " << i << ": dataframe transmitido possui paridade PAR, mas EDC informa paridade IMPAR. " << endl;
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
      cout << "Erro na coluna " << i << ": dataframe transmitido possui paridade IMPAR, mas EDC informa paridade PAR." << endl;

    } else if (frame->edc_code[edc_index] == '0' && even_counter % 2 == 0) {
      cout << "Erro na coluna " << i << ": dataframe transmitido possui paridade PAR, mas EDC informa paridade IMPAR. " << endl;
    }
    even_counter = 0;
    edc_index++;
  }
}

void odd_parity_check(frame *frame) {

}

void crc_check(frame *frame) {

}

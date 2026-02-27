# PhotoWebBluetooth (Hardware)

Este repositório contém todos os arquivos de hardware, esquemas eletrônicos e modelos 3D para a construção do sistema **PhotoWebBluetooth32 (PWB32)**.

O PWB32 é um sistema photogate de seis canais de baixo custo, desenvolvido para laboratórios de ensino de Física. Ele utiliza um microcontrolador ESP32 para detectar a passagem de objetos por sensores infravermelhos e transmite os dados via Bluetooth para uma interface web.

> **Nota:** Este hardware foi desenvolvido como parte do Trabalho de Conclusão de Curso de **Felipe Ricobello**, com colaboração técnica de **Aline de Almeida Soares** e **Fernando Rezende Apolinário** (CCA/UFSCar). Para o código-fonte do firmware e da interface web, acesse o repositório [PhotoWebBluetooth32](https://github.com/jocoteles/PhotoWebBluetooth32).

---

## 🛠 Conteúdo do Repositório

Este repositório está organizado nas seguintes pastas:

*   **`/PCB`**: Contém os esquemas do circuito, layout da PCB (se aplicável) e a lista de componentes (BOM) detalhada.
*   **`/STL`**: Inclui os arquivos STL e, possivelmente, os arquivos de projeto (ex: Fusion 360, OpenSCAD) para os cabeçotes dos sensores photogate e a caixa de acondicionamento do ESP32.
*   **`/DOCS`**: Apresenta diagramas de montagem, fotos do sistema finalizado e qualquer outra documentação relevante para a construção.

---

## 📋 Lista de Materiais (BOM - Bill of Materials)

Para montar uma unidade completa de 6 canais do PhotoWebBluetooth32, você precisará dos seguintes componentes:

### Eletrônica Principal
*   1x **ESP32 DevKit V1** (com 30 pinos, ou modelo compatível).
*   6x **Pares Infravermelhos** (compostos por 1x LED IR emissor e 1x Fototransistor receptor cada).
*   6x **Resistores para os LEDs IR** (valor típico: 150Ω a 220Ω, dimensionado conforme a corrente desejada e tensão de alimentação do LED).
*   6x **Resistores de pull-up para os fototransistores** (valor típico: 10kΩ).
*   **Cabos de conexão:** Fios para ligar os sensores ao ESP32 (recomenda-se cabos blindados ou do tipo flat para os sensores para maior organização e menos ruído).
*   **Conectores (opcional):** Conectores de 3 pinos (ex: JST, GX16) para os sensores para facilitar a modularidade e substituição.
*   **Protoboard ou PCB:** Para a montagem do circuito de condicionamento de sinal e conexão dos sensores.

### Estrutura (Impressão 3D)
*   6x **Corpos de cabeçotes de sensores** (design em "U" disponível em `/modelagem-3d`, para garantir o alinhamento do feixe IR).
*   1x **Caixa de controle** (disponível em `/modelagem-33`, para abrigar o ESP32 e as conexões principais).
*   **Parafusos e porcas** para fixação (detalhes no manual de montagem).

---

## 🔌 Pinagem e Circuito

O firmware padrão do PhotoWebBluetooth32 espera que os sensores photogate sejam conectados aos seguintes pinos analógicos do ESP32:

| Canal | Pino ESP32 (ADC) | Função |
| :--- | :--- | :--- | :--- |
| Canal 1 | GPIO 34 | Sensor Photogate 1 |
| Canal 2 | GPIO 35 | Sensor Photogate 2 |
| Canal 3 | GPIO 32 | Sensor Photogate 3 |
| Canal 4 | GPIO 33 | Sensor Photogate 4 | 
| Canal 5 | GPIO 25 | Sensor Photogate 5 | 
| Canal 6 | GPIO 26 | Sensor Photogate 6 |

**Diagrama Básico do Sensor Photogate:**

Cada sensor photogate consiste em um LED infravermelho e um fototransistor. O fototransistor é conectado em uma configuração de divisor de tensão, onde sua resistência varia drasticamente com a incidência de luz IR.

Quando um objeto passa e bloqueia o feixe IR, o fototransistor para de conduzir, e a tensão no pino ADC sobe (ou desce, dependendo da configuração). O firmware detecta essa mudança para registrar o evento.

Para o esquema detalhado e layout da PCB, consulte a pasta `/PCB`.

---

## 📐 Montagem Física

1.  **Impressão 3D:** Imprima todas as peças estruturais (`cabeçotes`, `caixa_controle`) utilizando os arquivos STL fornecidos em `/STL`. Recomenda-se PETG ou ABS para durabilidade.
2.  **Montagem da Eletrônica:**
    *   Soldar os resistores e o fototransistor para cada cabeçote de sensor.
    *   Montar o circuito de condicionamento de sinal (se houver) em uma protoboard ou PCB.
    *   Conectar os fios dos 6 sensores aos pinos do ESP32 conforme a tabela de pinagem acima.
    *   Garantir boas conexões de alimentação (3.3V e GND) para o ESP32 e os sensores.
3.  **Acondicionamento:**
    *   Encaixe o ESP32 dentro da caixa de controle impressa.
    *   Passe os cabos dos sensores pelas aberturas da caixa e conecte-os de forma organizada.
    *   Certifique-se de que a antena Wi-Fi/Bluetooth do ESP32 não esteja obstruída por componentes metálicos para garantir a melhor conectividade.
4.  **Montagem dos Cabeçotes:** Insira o LED IR e o fototransistor em seus respectivos orifícios nos cabeçotes impressos. O design em "U" deve facilitar o alinhamento.
5.  **Verificação:** Antes de fechar a caixa, faça uma verificação visual de todas as conexões e soldas.

Para um guia de montagem passo a passo com fotos, consulte a pasta `/documentacao`.

---

## 🚀 Como Integrar com o Software

Após a montagem do hardware:

1.  **Grave o Firmware:** Siga as instruções no repositório [PhotoWebBluetooth32](https://github.com/jocoteles/PhotoWebBluetooth32) para compilar e gravar o firmware `PWB32Server.ino` no seu ESP32.
2.  **Teste com a PWA:** Acesse a Progressive Web App (PWA) em [https://jocoteles.github.io/PhotoWebBluetooth32/](https://jocoteles.github.io/PhotoWebBluetooth32/) com um navegador compatível (Chrome, Edge, Opera em desktop ou Android).
3.  **Calibração:** Utilize a aba "Config" da PWA para ajustar os níveis de trigger de cada canal do photogate conforme as condições de luz do ambiente e a opacidade dos objetos a serem detectados.

---

## 📄 Licença

Este projeto de hardware é liberado sob uma licença de hardware aberto (ex: CERN OHL-P ou Creative Commons Attribution-ShareAlike 4.0 International). Sinta-se à vontade para modificar e melhorar os designs, desde que cite os autores originais.

---
*Desenvolvido para os Laboratórios de Ensino de Física do [CCA/UFSCar](https://www.fisicaararas.ufscar.br/pt-br).*
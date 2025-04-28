# Sistema ESP32 de Aquisição de Photogate com Buffer de Alta Velocidade

## Visão Geral

Esta é uma implementação paralela ao projeto principal que testa um sistema para adquirir dados de até seis sensores photogate analógicos em alta velocidade usando um microcontrolador ESP32. Os dados são armazenados temporariamente (buffer) localmente no ESP32 e, em seguida, transmitidos em blocos via WebSocket para uma aplicação web simples para visualização em tempo real usando Chart.js.

O objetivo principal é explorar a taxa de amostragem máxima alcançável, mantendo uma transmissão de dados confiável através de uma conexão WiFi local (ESP32 atuando como Ponto de Acesso). Ele se concentra em técnicas de buffering para desacoplar o processo de amostragem de alta frequência do processo potencialmente mais lento de transmissão pela rede.

**Observação:** O código do ESP32 inclui uma função `osc()` artificial adicionada às leituras analógicas. Isso serve puramente para fins de teste, para simular sinais dinâmicos dos sensores sem a necessidade de photogates físicos durante o desenvolvimento. Remova ou comente a parte `+ osc(...)` na função `loop()` ao usar sensores reais.

## Recursos

*   Lê até 6 canais de sensores analógicos.
*   Foco em altas taxas de amostragem usando delays de nível de microssegundos.
*   Implementa buffering de dados para lidar com possíveis incompatibilidades entre as velocidades de aquisição e transmissão.
*   Transmite dados eficientemente em blocos binários usando WebSockets.
*   O ESP32 opera em modo Ponto de Acesso (AP) para conexão direta sem um roteador externo.
*   Aplicação web simples (`index.html`) conecta-se ao ESP32, analisa dados binários e exibe:
    *   Gráfico de linha em tempo real dos 6 valores dos sensores vs. tempo (usando Chart.js).
    *   Log dos blocos de dados recebidos (mostrando detalhes da primeira/última amostra por bloco).
*   Inclui uma estrutura teórica (veja seção Ajuste de Desempenho) para otimizar o tamanho do buffer e o intervalo de amostragem.

## Requisitos de Hardware

*   Placa de Desenvolvimento ESP32 (ex: ESP32-WROOM-DA, NodeMCU-32S, etc.)
*   Até 6 Sensores Analógicos (ex: Photogates com saída analógica, potenciômetros para teste)
*   Fios de Conexão
*   Fonte de Alimentação para ESP32
*   Computador ou Dispositivo Móvel com WiFi e um Navegador Web

## Requisitos de Software

**Firmware ESP32 (`photogate_speedtest.ino`)**

*   Arduino IDE ou PlatformIO
*   Pacote de Suporte à Placa ESP32 (Testado com v2.0.x ou posterior, verifique comentários no `.ino` para especificidades, se fornecido)
*   Bibliotecas:
    *   `ESPAsyncWebServer` (Instalar via Gerenciador de Bibliotecas)
    *   `AsyncTCP` (Dependência do ESPAsyncWebServer, geralmente instalado automaticamente)
*   (Opcional) Monitor Serial para saída de depuração.

**Aplicação Cliente Web (`index.html`)**

*   Um Navegador Web moderno que suporte WebSockets e JavaScript (Chrome, Firefox, Edge, Safari).
*   Nenhuma instalação necessária, apenas abra o arquivo HTML.

## Como Funciona

1.  **Configuração do ESP32:**
    *   O ESP32 inicializa o WiFi no modo Ponto de Acesso (AP) com um SSID (`ESP32_Photogate`) e senha (`12345678`) predefinidos.
    *   Inicia um Servidor Web assíncrono e um servidor WebSocket escutando em `/ws`.
    *   O caminho raiz (`/`) serve uma mensagem de texto simples indicando que o servidor está rodando.
    *   Os pinos dos sensores são configurados como entradas.
    *   Um timestamp inicial (`t0`) é registrado.

2.  **Aquisição e Buffering de Dados (ESP32 `loop()`):**
    *   O código verifica se há algum cliente WebSocket conectado.
    *   Se clientes estiverem conectados:
        *   Lê os valores analógicos dos 6 pinos dos sensores.
        *   (Nota: O código de teste adiciona uma oscilação artificial aqui).
        *   Registra o tempo atual relativo ao início (`millis() - t0`).
        *   Esses valores (6 leituras de sensor + 1 timestamp) são armazenados no array `sensorBuffer` no índice atual `bufferIndex`, formando um `SensorDataPacket`.
        *   O `bufferIndex` é incrementado.
        *   Um pequeno delay (`delayMicroseconds(SAMPLE_INTERVAL_US)`) é introduzido para controlar a taxa de amostragem.
        *   Se o `bufferIndex` atingir `NUM_SAMPLES_PER_CHUNK` (significando que o buffer está cheio):
            *   O `sensorBuffer` inteiro (contendo `NUM_SAMPLES_PER_CHUNK` pacotes) é enviado como uma única mensagem binária para todos os clientes WebSocket conectados usando `ws.binaryAll()`.
            *   O `bufferIndex` é resetado para 0 para começar a preencher o buffer novamente.
    *   Se nenhum cliente estiver conectado, o `bufferIndex` é resetado, e o ESP32 espera brevemente para evitar espera ocupada (busy-waiting).

3.  **Aplicação Web (`index.html`):**
    *   O arquivo HTML configura a estrutura básica da página, indicadores de status, botões, um canvas do Chart.js e uma área de log de dados.
    *   O código JavaScript:
        *   Define o endereço IP do ESP32 (fixo como `192.168.4.1`, o IP padrão do AP).
        *   Fornece botões "Conectar" e "Desconectar" para gerenciar a conexão WebSocket.
        *   Define `socket.binaryType = "arraybuffer"` para receber dados binários.
        *   **Na Conexão:** Reseta o gráfico e o log de dados.
        *   **Ao Receber Mensagem:**
            *   Verifica se os dados recebidos são um `ArrayBuffer`.
            *   Verifica se o tamanho do buffer é um múltiplo de `SINGLE_PACKET_SIZE` (16 bytes).
            *   Usa um `DataView` para iterar através do `ArrayBuffer`, lendo cada `SensorDataPacket` de acordo com sua estrutura (6 `uint16_t` e 1 `uint32_t`, usando formato little-endian `true`).
            *   Para cada amostra decodificada, adiciona os pontos de dados aos conjuntos de dados correspondentes no objeto `chartData` e adiciona o timestamp (convertido para segundos) aos rótulos (labels).
            *   Registra os detalhes da primeira e última amostra dentro do bloco recebido na área `dataOutput`.
            *   Após processar todas as amostras no bloco, chama `chart.update('none')` uma vez para redesenhar o gráfico eficientemente sem animações.
            *   Mantém um número máximo de pontos exibidos no gráfico (`MAX_CHART_POINTS`) e linhas no log (`MAX_LINES`) removendo dados antigos.
        *   **Ao Fechar/Erro:** Atualiza o indicador de status.

## Configuração (Código ESP32)

Você pode modificar estes parâmetros em `photogate_speedtest.ino`:

*   `ssid`: O nome da rede WiFi que o ESP32 criará.
*   `password`: A senha para a rede WiFi (defina como `NULL` ou `""` para uma rede aberta).
*   `SENSOR_PIN_1` a `SENSOR_PIN_6`: Pinos GPIO conectados aos seus sensores analógicos. Certifique-se de que são pinos ADC válidos na sua placa ESP32 específica.
*   `NUM_SAMPLES_PER_CHUNK` (`N`): O número de leituras completas do sensor (amostras) a serem acumuladas no buffer antes de enviar uma mensagem WebSocket. Valores maiores reduzem o overhead de rede por amostra, mas aumentam a latência e o uso de memória.
*   `SAMPLE_INTERVAL_US`: O delay alvo *entre* o início de ciclos de amostragem consecutivos, em microssegundos. Isso influencia diretamente a taxa de amostragem (Taxa ≈ 1.000.000 / `SAMPLE_INTERVAL_US` Hz). Veja a seção Ajuste de Desempenho para como escolher este valor otimamente.

## Ajuste de Desempenho: Balanceando Aquisição e Transmissão

O desafio principal é amostrar dados o mais rápido possível (`T_acq`) sem sobrecarregar a transmissão pela rede (`T_tx`). Se o buffer encher mais rápido do que pode ser enviado, dados serão eventualmente perdidos ou o sistema se tornará instável. Um aspecto fundamental da ESP32 é que ela consegue transmitir o pacote de dados pela rede em paralelo com a leitura dos pinos analógicos e processamento dos dados. É essa capacidade que possibilita a transmissão em alta velocidade via bufferização.

**Definições:**

*   `N`: `NUM_SAMPLES_PER_CHUNK` - Número de amostras por buffer.
*   `tr`: Tempo para ler todos os 6 pinos analógicos uma vez (microssegundos). *Deve ser medido experimentalmente* no seu hardware (ex: usando `micros()` em torno do bloco `analogRead`).
*   `t2`: Delay artificial *efetivo* adicionado por ciclo de amostragem (microssegundos). Neste código, `SAMPLE_INTERVAL_US` representa o intervalo *total*, então o delay *adicionado* é `t2 = SAMPLE_INTERVAL_US - tr`. Se `SAMPLE_INTERVAL_US < tr`, então o `t2` efetivo é 0, e o intervalo real é limitado por `tr`.
*   `B`: `SINGLE_PACKET_SIZE` - Tamanho de um `SensorDataPacket` em bytes (16 bytes neste código).
*   `T_acq`: Tempo para adquirir `N` amostras (encher um buffer).
    *   `T_acq = N * max(tr, SAMPLE_INTERVAL_US)`
    *   Alternativamente, usando o delay *adicionado* `t2`: `T_acq = N * (tr + t2)` onde `t2 = max(0, SAMPLE_INTERVAL_US - tr)`.
*   `t0`: Overhead fixo por transmissão WebSocket (microssegundos). Inclui enquadramento WebSocket, cabeçalhos TCP/IP, processamento da biblioteca, etc. *Deve ser estimado.*
*   `k`: Fator de tempo de transmissão por byte (microssegundos/byte). Depende da velocidade do WiFi, processamento do cliente, etc. *Deve ser estimado.*
*   `T_tx`: Tempo total para processar a transmissão de um buffer de `N` amostras.
    *   `T_tx = t0 + (k * N * B)`

**Condição para Estabilidade:**

Para evitar perda de dados, o tempo necessário para transmitir um buffer (`T_tx`) deve ser menor ou igual ao tempo que leva para adquirir o *próximo* buffer (`T_acq`):

`T_tx <= T_acq`

**Otimizando `SAMPLE_INTERVAL_US`:**

Substituindo as expressões para `T_tx` e `T_acq`:

`t0 + k * N * B <= N * max(tr, SAMPLE_INTERVAL_US)`

Queremos encontrar o `SAMPLE_INTERVAL_US` *mínimo* que satisfaz esta condição. Analisando a desigualdade:

`(t0 / N) + k * B <= max(tr, SAMPLE_INTERVAL_US)`

Isso significa que `SAMPLE_INTERVAL_US` deve ser maior ou igual ao termo à esquerda. Ele também deve ser intrinsecamente maior ou igual ao tempo que leva para realmente realizar as leituras (`tr`). Portanto:

`SAMPLE_INTERVAL_US_min = max(tr, (t0 / N) + k * B)`

**Interpretação e Passos Práticos:**

1.  **Meça `tr`:** Use `micros()` no seu código ESP32 antes do primeiro `analogRead()` e depois do último `analogRead()` dentro do bloco de amostragem para obter uma medida precisa de `tr`.
2.  **Estime `t0` e `k`:** Esta é a parte mais difícil. Comece com estimativas aproximadas. `t0` pode estar na faixa de 1000-5000 µs (1-5 ms). `k` depende fortemente da taxa de dados efetiva do WiFi. Para uma taxa teórica de 1 Mbps (~125 KB/s), `k` seria em torno de 8 µs/byte. Na prática, pode ser maior devido a overheads.
3.  **Escolha `N` (`NUM_SAMPLES_PER_CHUNK`):** Comece com um valor moderado (ex: 10, 20, 50).
4.  **Calcule `SAMPLE_INTERVAL_US_min`:** Use a fórmula `max(tr, (t0 / N) + k * B)`.
5.  **Defina `SAMPLE_INTERVAL_US`:** Defina o valor no seu código para `SAMPLE_INTERVAL_US_min`, potencialmente adicionando uma pequena margem de segurança (ex: 10-20% ou um valor fixo de 50-100 µs).
6.  **Teste Experimentalmente:** Execute o sistema. Monitore a saída serial para erros (se algum for adicionado) e observe a taxa de dados e a estabilidade no cliente web. Verifique se os timestamps recebidos no cliente estão aumentando de forma constante, sem grandes lacunas (indicando blocos perdidos).
7.  **Ajuste e Itere:**
    *   **Se instável (lacunas de dados, lag):** Suas estimativas para `t0`/`k` podem ser muito otimistas, ou `N` é muito pequeno em relação ao overhead. Aumente ligeiramente `SAMPLE_INTERVAL_US`, ou tente aumentar `N` (e recalcular o `SAMPLE_INTERVAL_US` necessário).
    *   **Se estável:** Você pode tentar reduzir *cuidadosamente* `SAMPLE_INTERVAL_US` em direção ao mínimo calculado, ou tentar aumentar `N` ainda mais para potencialmente melhorar a eficiência (menor overhead por amostra), recalculando `SAMPLE_INTERVAL_US_min` a cada vez.

Este modelo fornece um ponto de partida quantitativo para ajustar `NUM_SAMPLES_PER_CHUNK` e `SAMPLE_INTERVAL_US` para alcançar o melhor desempenho possível para seu hardware e condições de rede específicas.

## Instruções de Uso

1.  **Montagem do Hardware:** Conecte seus sensores analógicos aos pinos do ESP32 definidos no arquivo `.ino` (padrão: 32-37). Alimente o ESP32.
2.  **Upload do Firmware:**
    *   Abra `photogate_speedtest.ino` na Arduino IDE ou PlatformIO.
    *   Modifique `ssid`, `password` e os pinos dos sensores, se necessário.
    *   Selecione sua placa ESP32 e a porta.
    *   Carregue o sketch para o ESP32.
3.  **Monitorar (Opcional):** Abra o Monitor Serial (Baud Rate: 115200) para ver mensagens de inicialização, o endereço IP do AP (deve ser `192.168.4.1`) e eventos de conexão.
4.  **Conectar Cliente:**
    *   No seu computador ou dispositivo móvel, conecte-se à rede WiFi criada pelo ESP32 (ex: `ESP32_Photogate`).
5.  **Abrir Aplicação Web:**
    *   Abra o arquivo `index.html` no seu navegador web.
6.  **Conectar WebSocket:**
    *   Verifique se o endereço IP no JavaScript de `index.html` (`esp32Ip`) corresponde ao IP do AP do ESP32 (normalmente `192.168.4.1`).
    *   Clique no botão "Conectar" na página web.
7.  **Observar Dados:**
    *   O status deve mudar para "Conectado".
    *   A área "Blocos de Dados Recebidos" deve começar a mostrar mensagens de log para os blocos de dados recebidos.
    *   O "Gráfico ao Vivo" deve exibir as 6 leituras dos sensores plotadas contra o tempo.

## Melhorias Potenciais

*   Adicionar tratamento de erros mais robusto (ex: verificar valores de retorno de `analogRead`, lidar com erros de WebSocket de forma mais graciosa).
*   Implementar lógica de reconexão no lado do cliente.
*   Adicionar funcionalidade de salvamento/exportação de dados à aplicação web.
*   Explorar o uso do segundo ADC no ESP32 para leituras paralelas potencialmente mais rápidas (requer seleção cuidadosa de pinos e alterações no código).
*   Investigar o uso de UDP em vez de WebSocket para overhead potencialmente menor, ao custo da entrega garantida.
*   Adicionar mecanismos de disparo (triggering) baseados nos valores dos sensores.
*   Implementar calibração dos sensores.
*   Otimizar a velocidade do `analogRead` ajustando a atenuação ou resolução do ADC (se aceitável).

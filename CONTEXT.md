# Substituição de Páginas

Simulação de políticas de substituição de páginas sobre traces de acesso à memória,
para comparar quantas falhas de página cada política produz com um número fixo de
frames.

## Language

### Memória

**Frame** (`frame`):
Espaço da memória física com capacidade para exatamente uma página.
_Avoid_: quadro, moldura, quadro de página

**Número de frames** (`frame_count`):
Quantidade de frames disponíveis ao processo durante uma simulação. É fixa do início
ao fim e todos começam vazios.
_Avoid_: tamanho da memória, frames livres

**Página** (`page`):
Bloco de 4096 bytes do espaço de endereçamento virtual, identificado pelo endereço
sem os 12 bits de deslocamento. É a unidade que ocupa um frame.
_Avoid_: bloco

### Trace

**Trace** (`trace`):
Sequência ordenada de acessos de um programa real, lida de um arquivo.
_Avoid_: log, arquivo de entrada

**Acesso** (`access`):
Uma linha do trace: um endereço de 32 bits e um tipo (leitura ou escrita). O tipo
é ignorado — leitura e escrita contam igual.
_Avoid_: referência (colide com bit de referência), requisição

**Falha de página** (`page_fault`):
Acesso cuja página não está em nenhum frame, incluindo as falhas compulsórias do
início. É a única métrica comparada entre políticas.
_Avoid_: miss, page miss

### Políticas

**Política de substituição** (`simulate_<política>`):
Regra que escolhe a vítima quando ocorre uma falha de página e todos os frames estão
ocupados. No código, cada política é uma função livre `simulate_<política>`; a
interface comum entre elas é a assinatura compartilhada — páginas do trace e número
de frames de entrada, número de falhas de página de retorno (o LRU aproximado recebe
ainda N e I).
_Avoid_: algoritmo de paginação, estratégia

**Vítima** (`victim`):
Página escolhida pela política para deixar seu frame e dar lugar à página que faltou.
_Avoid_: página expulsa, página removida

**FIFO** (`simulate_fifo`):
Política cuja vítima é a página carregada há mais tempo, independentemente do uso.
_Avoid_: fila

**OPT** (`simulate_opt`):
Política ótima (Belady) cuja vítima é a página cujo próximo uso está mais distante no
futuro do trace — ou que nunca mais será usada. Serve de limite inferior.
_Avoid_: ótimo, MIN, Belady

**Próximo uso** (`next_use`):
Posição, no trace, do próximo acesso a uma página a partir do acesso atual.
_Avoid_: distância futura

**LRU aproximado** (`simulate_lru_approx`):
Política de envelhecimento (aging): a vítima é a página de menor histórico,
desempatando por bit de referência desligado e, depois, pela carregada há mais tempo.
_Avoid_: LRU (é o exato, fora da comparação), aging, clock, segunda chance

**Bit de referência** (`ref_bit`):
Bit de uma página em frame que é ligado a cada acesso a ela e desligado a cada
envelhecimento.
_Avoid_: bit R, bit de uso, bit de acesso

**Histórico** (`history`):
Registro de N bits de uma página em frame; o bit mais significativo representa o
intervalo de envelhecimento mais recente. Página recém-carregada começa com histórico
zerado.
_Avoid_: contador, idade, bits adicionais

**Bits de histórico** (`history_bits`, N):
Largura do histórico — parâmetro do LRU aproximado escolhido pelo experimento.
_Avoid_: tamanho do contador

**Envelhecimento** (`age`):
Operação aplicada a todas as páginas em frame: o histórico desloca um bit à direita,
o bit de referência entra como bit mais significativo e é então desligado.
_Avoid_: deslocamento, shift, tick

**Intervalo de envelhecimento** (`aging_interval`, I):
Número de acessos do trace entre dois envelhecimentos consecutivos — parâmetro do LRU
aproximado.
_Avoid_: intervalo de deslocamento, período, quantum

### Avaliação

**Simulação** (`simulation`):
Uma execução de um trace sob uma política, um número de frames e (no LRU aproximado)
um par N/I, produzindo um único número de falhas de página.
_Avoid_: rodada, teste, benchmark

**Experimento** (`experiment`):
Conjunto de simulações que varia um ou mais parâmetros para produzir um gráfico ou
tabela do artigo.
_Avoid_: bateria, teste, benchmark

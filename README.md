# Mini-Kernel Multithread — Escalonamento FCFS, RR e Prioridade Preemptiva

Alunos: Leonardo Vitório Rezende (2024102899) e Mateus Silva Lizardo (2024101222)

Trabalho Prático de Sistemas Operacionais (INF15980) — UFES. Simula um
escalonador de processos *multithread* em C, usando threads POSIX
(`pthread`), com três políticas de escalonamento e duas versões: um
sistema monoprocessador (1 CPU) e um multiprocessado (2 CPUs).

## Compilação

O executável gerado se chama `trabSO` nas duas versões.

```sh
make monoprocessador   # compila com 1 processador (NUM_CPUS=1)
make multiprocessador  # compila com 2 processadores (NUM_CPUS=2)
```

`make` (sem alvo) equivale a `make monoprocessador`. `make clean` remove o
executável. A opção `-DNUM_CPUS` do Makefile é o único parâmetro que
diferencia as duas versões — o mesmo código-fonte gera ambas.

## Execução

```sh
./trabSO <arquivo_de_entrada>
```

O programa não lê nada do teclado e não imprime nada na tela: toda a saída é
gravada no arquivo `log_execucao_minikernel.txt` (criado no diretório atual,
sobrescrito a cada execução).

## Formato de entrada

Arquivo de texto com números inteiros. A primeira linha contém `n`, a
quantidade de processos. Em seguida, para cada um dos `n` processos, quatro
valores (um por linha):

1. Duração total de execução, em milissegundos
2. Prioridade, de 1 (maior) a 5 (menor)
3. Número de threads do processo
4. Tempo de chegada, em milissegundos relativos ao início da execução

A última linha indica a política de escalonamento: `1` = FCFS, `2` = Round
Robin, `3` = Prioridade Preemptiva.

Exemplo (3 processos, política FCFS):

```
3
1000
3
2
0
2000
1
1
200
1500
2
1
100
1
```

O PID de cada processo é atribuído na ordem de leitura (1, 2, 3, ...), não é
o `getpid()` do sistema.

## Formato de saída (`log_execucao_minikernel.txt`)

> Nota: o enunciado é inconsistente quanto ao nome do arquivo — as seções
> 2.6/2.10 citam `escalonador_log_minikernel.txt`, enquanto a seção 4.2
> (Saída) especifica `log_execucao_minikernel.txt`. Adotamos este último,
> conforme a seção 4.2, que é a que define o nome do arquivo de saída
> avaliado. (Os casos de teste fornecem apenas entrada e saída esperada, ou
> seja, o conteúdo do log — não o nome do arquivo.)

Cada evento de escalonamento vira uma linha. No modo monoprocessador, para o
exemplo acima:

```
[FCFS] Executando processo PID 1
[FCFS] Processo PID 1 finalizado
[FCFS] Executando processo PID 3
[FCFS] Processo PID 3 finalizado
[FCFS] Executando processo PID 2
[FCFS] Processo PID 2 finalizado
Escalonador terminou execução de todos processos
```

As mensagens variam conforme a política:

- `[FCFS] Executando processo PID <pid>`
- `[RR] Executando processo PID <pid> com quantum <q>ms`
- `[PRIORITY] Executando processo PID <pid> prioridade <p>`
- `[<POL>] Processo PID <pid> finalizado`

No modo multiprocessador, cada linha `Executando` recebe o sufixo
` // processador <id>` (0 ou 1), indicando qual CPU despachou o processo.

## Arquitetura

O código é modularizado por responsabilidade:

- **PCB (`PCB.c`/`PCB.h`)** — Bloco de Controle de Processo. Guarda `pid`,
  `process_len`, `remaining_time`, `priority`, `num_threads`, `start_time`,
  o estado (`READY`/`RUNNING`/`FINISHED`), o mutex e a variável de condição
  do processo e o vetor `thread_ids`. Também contém a rotina executada por
  cada thread do processo (`routine`), que simula o trabalho com `usleep` e
  desconta do `remaining_time` a fatia de tempo correspondente
  (`QUANTUM / num_threads`, pois as threads dividem a duração do processo).
  A primeira thread a zerar `remaining_time` marca o processo como
  `FINISHED` e sinaliza as demais.

- **TCB (`TCB.c`/`TCB.h`)** — Bloco de Controle de Thread. Estrutura leve com
  um ponteiro para o PCB dono e o `thread_index` local, criada para cada
  thread do processo.

- **Fila de prontos (`fila.c`/`fila.h`)** — a *ready queue*. Serve às três
  políticas com uma única estrutura: para FCFS/RR comporta-se como fila
  circular (inserção no fim, remoção do início, e remoção arbitrária para
  preempção); para Prioridade, as mesmas operações usam um **heap mínimo**
  por `priority` (`insereHeap`/`removeHeap`/`maiorPrioridade`). Cresce
  dinamicamente e sinaliza `generator_done` quando todos os processos já
  chegaram.

- **Escalonador (`escalonador.c`/`escalonador.h`)** — a thread escalonadora.
  Retira o próximo processo da fila conforme a política, muda seu estado
  para `RUNNING`, faz *broadcast* na CV do processo para acordar as threads
  e aguarda o término (ou o fim do quantum). Também concentra a escrita no
  log (protegida por mutex).

- **`main.c`** — lê a entrada, ordena os processos por tempo de chegada,
  cria a(s) thread(s) escalonadora(s), simula a chegada dos processos no
  tempo certo (aguardando com `usleep` até cada `start_time` e só então
  criando as threads e enfileirando o processo), e ao final faz `join` de
  tudo e libera a memória.

### As três políticas

- **FCFS** (`escalonaFCFS`): despacha o processo e espera ele terminar por
  completo antes de pegar o próximo. Sem preempção.

- **Round Robin** (`escalonaRR`): despacha o processo e cede a CPU após um
  quantum fixo (`QUANTUM_PADRAO_MS = 500ms`). Se ainda houver tempo
  restante, o processo volta ao fim da fila; caso contrário, é finalizado.
  Como manda a spec, o processo só é preemptado ao fim do quantum, mesmo que
  precise de menos tempo.

- **Prioridade Preemptiva** (`escalonaPrioridade`): usa o heap para sempre
  executar o processo de maior prioridade. A cada ciclo verifica, via
  `maiorPrioridade`, se chegou alguém mais prioritário; em caso afirmativo,
  reinsere o processo atual no heap (`insereHeap`) e passa a executar o novo.

## Sincronização

Toda a coordenação usa mutexes e variáveis de condição POSIX, sem
*busy-wait* desnecessário:

- **Mutex + CV do processo (PCB):** protegem `state` e `remaining_time`
  contra acesso concorrente das threads e do escalonador. As threads
  bloqueiam na CV enquanto o estado não for `RUNNING`; o escalonador acorda
  todas com `pthread_cond_broadcast` ao despachar, e a CV também sinaliza o
  fim do processo.

- **Mutex + CV da fila de prontos:** garantem que só um agente (escalonador
  ou o produtor de processos em `main`) manipule a fila por vez. A CV
  (`cond`) faz o escalonador dormir quando a fila está vazia e acordar
  quando um novo processo chega ou quando `generator_done` é sinalizado.

- **Mutex do log:** serializa a escrita no arquivo, essencial no modo
  multiprocessador, onde as duas CPUs escrevem no mesmo arquivo.

A ordem de travamento mantém "mutex do processo" e "mutex da fila" de forma
disjunta, evitando *deadlock*. Estados que decidem o fluxo (ex.: "processo
terminou?") são lidos ainda sob o *lock* do PCB.

## Modo multiprocessador (2 CPUs)

Na versão `multiprocessador`, `main.c` cria **duas** threads escalonadoras
(`cpu_id` 0 e 1) que rodam a mesma rotina sobre a **mesma** fila de prontos
compartilhada. A exclusão mútua no mutex da fila é o que impede as duas CPUs
de pegarem o mesmo processo — não há sistema extra de "atribuição". Cada
escalonador conhece o seu par via `defineParEscalonador` (ambos são ligados
antes de qualquer thread iniciar, para evitar uma corrida de inicialização).

### Decisão da CPU ajudante

Quando uma CPU fica ociosa (fila vazia) mas a outra está executando um
processo com **mais de uma thread**, a CPU ociosa não fica parada: ela
"ajuda" executando outra thread do mesmo processo — coerente com a
modelagem 1:1 de threads e com a observação da spec de que dois
processadores atendem threads do *mesmo* processo em paralelo **somente
quando não há mais de um processo pronto na fila**.

Isso é implementado por `ajudaProcessoParalelo` (a única função nova de
coordenação): a CPU ociosa consulta `esc->par->current_process` (sob o mutex
da fila); se ele está `RUNNING` e tem `num_threads >= 2`, registra sua
própria linha `Executando ... // processador <id>` e passa a acompanhar esse
processo. Ela **abandona** a ajuda assim que (a) o processo termina, (b) o
dono passa a ser responsável por outro processo, ou (c) surge um novo
processo pronto na fila — pois a prioridade dos processadores é dar vazão a
*processos*, atendendo threads paralelas apenas na folga.

## Qualidade

- Passa nos casos de teste públicos do modo monoprocessador.
- Executa limpo no **Valgrind** (sem vazamento de memória) e no **Helgrind**
  (sem condições de corrida) em ambos os modos, para os casos de teste
  fornecidos.
- No modo multiprocessador, a *ordem* exata das linhas de log em torno de
  eventos simultâneos pode variar entre execuções (as duas CPUs correm para
  registrar o mesmo evento), o que é esperado e coerente com a própria
  observação do gabarito (`saidas/multi/obs.txt`).

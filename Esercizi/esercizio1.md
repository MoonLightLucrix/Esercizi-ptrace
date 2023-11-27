## Esercizio 1
Creare un Array sulla quale verra installato un breakpoint, il processo principale poi sceglie una locazione a caso e ci scrive un valore sopra.<br>
Nella funzione ```hook()``` il valore handled verrà impostato all'indirizzo dell'allocazione che è stata modificata.<br>
Inoltre, nella allocazione modificata sarà inserito il valore 99.
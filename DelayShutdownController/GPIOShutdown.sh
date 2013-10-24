#!/bin/sh
#----------------------------------------------------------#
#                     GPIOshutdown.sh                      #
# Original Author: Willem Aandewiel (Willem@Aandewiel.nl)  #
# Modified by A.Franco                                     #
# ver.0.1   18/10/2013                                     #
#----------------------------------------------------------#

#  Verifica che il demone non sia gia' in esecuzione  #
BASENAME=`basename ${0}`
PIDFILE="/var/run/${BASENAME%.sh}.pid"
if [ -f ${PIDFILE} ]; then
    echo "${BASENAME} allready running..."
    exit
else
    echo ${$} > ${PIDFILE}
fi
#
# =================  SETTING =============================
#    ---------  Piedini dei connettori P1 P5 ------------ #
# Connettore P1
# 0,1,4,7,8,9,10,11,14,15,17,18,21,22,23,24, 25
#
# Connettore P5
# 29,31
#
#  Definisce il pin GPIO
GPIO_IN=29        # Input - da modificare asseconda del circuito
#
#  ----------- Tempi di ciclo -------------------------- #
SHUTDOWNDELAY=20    # Ritardo fra la caduta del segnale e l'avvio della procedura di Shutdown
POLLINGDELAY=2      # Sleep time, tempo di polling dello stato
# =========================================================


# ---------- Set up GPIO_IN e set to Input  -----------
echo "${GPIO_IN}" > /sys/class/gpio/export
echo "in" > /sys/class/gpio/gpio${GPIO_IN}/direction
#
# test dell'input prova per 10 secondi di poter leggere il pin 
I=0             
SW=`cat /sys/class/gpio/gpio${GPIO_IN}/value`
echo "value GPIO_${GPIO_IN} is [${SW}]"
while [ ${SW} -eq 0 ];
do
    I=$((I+1))
    if [ ${I} -gt 10 ]; then
        echo "GPIO_${GPIO_IN} non funziona. Programma abortito !"
        rm -f ${PIDFILE}
        exit
    fi
    SW=`cat /sys/class/gpio/gpio${GPIO_IN}/value`
    echo -n "${I}"
    sleep 1
done
#
# ---------------- Main program ------------------
S=POLLINGDELAY        # Variabile di appoggio
I=0        # Contatore 
while true
do
    sleep ${S}  # aspetta un po'
    SW=`cat /sys/class/gpio/gpio${GPIO_IN}/value`  # legge lo stato della linea
    if [ ${SW} -eq 0 ]  # la linea e' uguale a zero -> si deve spegnere
    then
        I=$((I+1))    # conta il tempo di mantenimento della linea bassa
        S=1           # riduce il polling ad un secondo
        if [ ${I} -gt SHUTDOWNDELAY ]
        then
            echo "Segnale di spegnimento durato per [${SHUTDOWNDELAY}] secondi"
            echo "Avvio della procedura di ShutDown !"
            I=0
            echo "${GPIO_IN}" > /sys/class/gpio/unexport  # Reset dell'input
            /sbin/shutdown -h now           # Esecuzione comando
            rm -f ${PIDFILE}                # Rimozione del PID file
            sleep 5                         # Stabilizzazione..
            exit                            # Fine loop
        fi
    else
        I=0        # Era un falso allarme, un glitch o un ripensamento
        S=POLLINGDELAY    # Ripristina il tempo di polling
    fi      
done
#
# Ripulisce il set 
echo "${GPIO_IN}" > /sys/class/gpio/unexport
# Fine
echo "Done.... Bye Bye !"
#
# ------------------------- EOF -------------------------------

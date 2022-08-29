Dans Mantra, qui est considérée à juste titre comme une des premières pièces importante du répertoire de la musique dite “en temps réel”, Stockhausen ne fait pas qu’étendre les possibilités sonores du duo de piano par des moyens musicaux (crotales) et électroacoustiques (dispositif analogiques : oscillateurs, modulateurs en anneaux, recepteur à ondes courtes) mais il intègre ces appareils dans le jeu des pianistes. C’est aux pianistes eux-même qu’il demande de manipuler ces dispositifs inhabituels. Comme ces machines analogiques ne sont plus disponibles actuellement, il faudra donc probablement toujours ré-interpreter l’instrumentarium de Mantra en utilisant les technologies contemporaines. Les potentiomètres rotatifs des générateurs d’ondes sinusoidales sont aujourd’hui remplacés par des tablettes tactiles, le “ring-modulateur” par un ordinateur et le poste de radio par un enregistrement des codes morse tels que l’on pouvait les capter sur les ondes courtes dans les années 1970.


## puredata patch
The patch runs under pure data version 0.52-2 - Darwin - 64 bit
The starting point for this patch is the Miller Puckette pdrp version (cf.http://msp.ucsd.edu/pdrp/latest/doc/)

## technical rider
the latest version of the tech rider is :
[fiche-techniqueMANTRA2019.pdf](https://github.com/slemouton/mantra2021/blob/master/Mantra-doc/fiche-techniqueMANTRA2019.pdf)

## touchosc

The pianists controls the oscillators using two ipads runnning touchosc.

Pour cette version, chaque pianiste manipule un iPad qui contrôle les frequences des oscillateurs générés par le Macintosh.

transfer *mantra_Master2022b.tosc*,  *mantra_1b2022.tosc* and *mantra_2b2022.tosc* to all the ipads

- Piano 1 interface :*mantra_1b2022.tosc* (3 pages)

    - 1 octave solb2/sol#3 (54/68)
    - registre aigu high -> A4 (81)	
    - kurzwellen

- Piano 2 interface : *mantra_2b2022.tosc* (3 pages)

    - 1 octave sib1/do3 (46/60)
    - high -> A5 (93)
    - low

- Electronic musician interface : *mantra_Master2022b.tosc* (5 pages)

    - during performance use the *concert* (middle tab) page

### touchosc and network (udp/osc) settings
Pour réaliser la connexion entre les ipad (logiciel touchOsc) et le macintosh (logiciel PureData), suivre les étapes suivantes (dans l’ordre) :

#### without wifi access point (wifi router) *recommanded*

* sur le Macintosh : location Manuelle
* exemple d’adresses fixes : 
* 129.255.1.1 pour le mac pureData (reception sur port 9997)
* 129.255.1.2 pour mon iphone (port 5006)
* double cliquer sur _mantra*.pd_
* connect the ipad to this wifi router
* attendre l'icone Wifi 
* lancer touchOSC, regler le Host -> la communication iPad->mac doit maintenant fonctionner
* open "pd controllers-IPADS" 
* verifier l’adresse ip de l’iPad: la communication doit maintenant fonctionner dans les deux sens

#### without wifi access point (adhoc connection)

* sur le Macintosh : location Manuelle
* exemple d’adresses fixes : 
* 129.255.1.1 pour le mac pureData (reception sur port 9997)
* 129.255.1.2 pour mon iphone (port 5006)
* sur le Macintosh : wifi : Turn Airport On -> Create Network
* double cliquer sur mantra2013.pd
* connect the ipad to this wifi network
* attendre l'icone Wifi 
* lancer touchOSC, regler le Host -> la communication iPad->mac doit maintenant fonctionner
* open "pd controllers-IPADS" 
* verifier l’adresse ip de l’iPad: la communication doit maintenant fonctionner dans les deux sens

## puredata patch checklist
- open *Mantra2021v.pd* with puredata (v0.52)
- press **reset**
- check the ipads IP addresses in the *controllers-IPADS* subpatch
- check that the ipads are communicating bi-directionnally with the patch

## performances
this patch has been publicly performed at the following concerts (and maybe more ...)

- 15/04/2012 - Chapelle Mejan - Arles -(Jean Frederic Neuburger - Jean François Heisser)
- 19/07/2012 - Abbaye aux Dames - festival de Saintes - Thomas Goepfer (JFN/JFH)
- 28/06/2013 - Ircam Summer Academy - Paris - 104 (Francoise - Green)
- 29/06/2013 - Ircam Summer Academy - Paris - 104 (Jean Frederic Neuburger - Jean François Heisser
- 26/03/2014 - Cite de la Musique - Paris - Augustin Muller - Cédric Pescia, Severin von Eckardstein, pianos
- 02/05/2014 - Renens Lausanne - Suisse - Augustin Muller - Cédric Pescia, Severin von Eckardstein, pianos
- 29/01/2016 - Cite de la Musique - Paris - Marco Stroppa - Pierre-Laurent Aimard  Pierre-Laurent Aimard et Tamara Stefanovich
- 17/09/2018 - Philharmonie - Berlin - Marco Stroppa - Pierre-Laurent Aimard  Pierre-Laurent Aimard et Tamara Stefanovich
- 28/07/2019 - festival de la Roque D'antheron - (JFN/JFH)
- 15/12/2019 - recording of the CD (Mirare) (conservatoire de Puteaux), tonmeister Jiri Heger (JFN/JFH)
- 28/06/2021 - Rencontres Musicales d'Evian - la Grange au Lac (JFN/JFH)
- 30/07/2021 - festival Messiaen - Eglise de la Grave (JFN/JFH)

*A venir :*

- 10/09/2022 - Festival Ravel (JFN/JFH)
- 16/01/2023 - Theatre des Bouffes du Nord (JFN/JFH)

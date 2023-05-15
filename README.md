# BE RESEAU
## TPs BE Reseau - 3 MIC - RAPPORT
### Eliot REVOL        	Imad BOUKEZZATA




## Contenu du dépôt
Vous trouverez dans ce dépôt la version 4.1 du BE réseaux.
Sont implémentés :
MICTCP incluant une phase de transfert de données sans garantie de fiabilité
étendu de la phase de transfert de données MICTCP-v1  de sorte à inclure une garantie de fiabilité totale via un mécanisme de reprise des pertes de type « Stop and  Wait »
garantie de fiabilité partielle « statique » via un mécanisme de reprise des pertes de type « Stop and Wait » à fiabilité partielle « pré câblée », i.e. dont le % de pertes admissibles est défini de façon statique
une phase d’établissement de connexion
une garantie de fiabilité partielle via un mécanisme de reprise des pertes de type « Stop and Wait » dont le % de pertes admissibles sera négocié durant la phase d’établissement de connexion


## Explications du code

V1: Implémentation de la fonction mic_tcp_send ainsi que toutes les primitive mic_tcp_socket et mic_tcp_bind, ainsi que l’implémentation de la fonction mic_tcp-recv sans envoie d'acquittement et sans traitement de pertes ,afin d’obtenir une phase de transfert sans garantie de fiabilité.


V2: Ajout d’un mécanisme de reprise de pertes  “Stop & Wait”, avec l’implémentation de numéro de série pour les messages envoyer ainsi que les les acquittements reçu via les variables num_ack et num_seq , qui sont incrémentés à chaque envoie , et vérifié afin de garantir une phase de transfert avec garantie de fiabilité.

V3: Ajout d’un pourcentage de perte admissible (loss rate) : on accepte avec un certain pourcentage de chance qu’il y est une erreur sans chercher à la corriger. Cela se fait lors de l’envoie (mic_tcp_send) : si on a pas reçu d’acquittement, on choisi un nombre au hasard entre 1 et 100 et si celui-ci est plus grand que le loss rate on renvoie le pdu, sinon on saute cette donnée et on la considère comme une perte admissible.

V4.1: Ajout de la phase d’établissement de connexion via un échange SYN / SYN-ACK / ACK entre le client et le serveur. Les pertes de paquets sont pris en compte lors de la phase d’établissement de connexion, le seul problème est lorsque le client reçoit le SYN–ACK mais le serveur ne reçoit pas le ACK final : dans ce cas, le serveur passe dans un processus d’attente au cas où le client n’aurait pas reçu le SYN-ACK et qu’il recevait de nouveau le SYN. Sinon, il est admis après un certain temps que le SYN-ACK a bien été reçu et que la connexion peut donc être établie : on a pas reçu à gérer le renvoie du ACK par le client en cas d’erreur d’envoie autrement. Ajout de la négociation du loss rate entre le client et le serveur : le serveur a un maximum prédéfini et le client a une loss rate défini au départ qu’il envoie via le pdu SYN, le serveur va ensuite choisir son loss rate ou celui du client en fonction de s’il respecte ses contraintes. Finalement, le pourcentage de pertes admissibles définitif est renvoyé au client via le PDU SYN-ACK qu’il confirme via le PDU ACK.


## Exécution du code

Le code s'exécute classiquement en faisait ./tsock_texte -p pour le puits et ./tsock_texte -s pour la source, de même ./tsock_video -p -t mictcp pour le puits et -s pour la vidéo

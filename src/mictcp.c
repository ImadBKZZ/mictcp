#include <mictcp.h>
#include <api/mictcp_core.h>
#include <stdlib.h>

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
 
mic_tcp_sock sock[10];

// definition des variables globales
int num_sock = 0;
int num_seq = 0;
int num_ack = 0;
int env_admissible_loss_rate = 0;
int rec_admissible_loss_rate = 0;
int max_admissible_loss_rate = 25;
 
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   if(result == -1) return -1; //Erreur initialisation
   
   //Initialisation d'un nouveau socket
   sock[num_sock].fd = num_sock;
   sock[num_sock].state = IDLE;
   num_sock++;
   
   //Choix du ratio de perte
   set_loss_rate(20);

   //Retourne le numéro de socket alloué (peut aller jusqu'à 10)
   return num_sock-1;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
	
   //Affectation d'une adresse au socket
   if(num_sock>socket){
        sock[socket].addr = addr;
        return 0;
   }
   
   return -1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    // En attente de réception du SYN
    while(sock[socket].state != SYN_RECEIVED){
    	sleep(0.1);
    }
    
    // Réception de SYN : en attente
    sock[socket].state = IDLE;
    mic_tcp_pdu PDU_SYN_ACK; //PDU à envoyer (SYN-ACK) pour informer de la mise en connection
    mic_tcp_pdu* PDU_ACK = malloc(sizeof(mic_tcp_pdu)); //PDU en attente de réception (ACK) pour informé de la connection de l'hôte
	
    //Allocation de la mémoire pour le PDU ACK de réception
    PDU_ACK->payload.data = malloc(sizeof(char)); 
    PDU_ACK->payload.size = sizeof(char);

    //Encapsulation SYN ACK (mise à jour des informations du PDU)
    PDU_SYN_ACK.header.syn=1; //SYN = 1
    PDU_SYN_ACK.header.ack=1; //ACK = 1
    PDU_SYN_ACK.header.source_port=sock[socket].addr.port; //Port source
    PDU_SYN_ACK.header.dest_port=addr->port; //Port destination


    //Verification que le Loss Rate appartient a la plage admissible (sinon on change sa valeur)
    if(rec_admissible_loss_rate>max_admissible_loss_rate) {
	rec_admissible_loss_rate = max_admissible_loss_rate;
    }

    //Allocation de la mémoire pour le PDU SYN ACK à envoyer
    PDU_SYN_ACK.payload.data = malloc(sizeof(rec_admissible_loss_rate));
    PDU_SYN_ACK.payload.size = sizeof(rec_admissible_loss_rate);
    *PDU_SYN_ACK.payload.data = rec_admissible_loss_rate;
        
    //Envoie du PDU SYN ACK, retourne -1 si erreur lors de l'envoie
    int res= IP_send(PDU_SYN_ACK,*addr);
    if (res==-1)  return -1; //Problème lors de l'envoie du PDU
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    int timeout = 20;

    //Tant que la connexion n'est pas établi (et donc qu'on a pas reçu de ACK) on renvoie un syn ack, jusqu'au timeout ou on accepte finalement la connexion (possible qu'on ne reçoive jamais le ack)
    while(sock[socket].state != ESTABLISHED) {
	    		//On envoie de nouveau le PDU SYN ACK car on a reçu SYN de nouveau (et donc le client n'a pas reçu de SYN-ACK)
			if(sock[socket].state == SYN_RECEIVED) {  
				sock[socket].state = IDLE;
				res= IP_send(PDU_SYN_ACK,*addr);
    				if (res==-1)  return -1; //Problème lors de l'envoie du PDU
			}
			sleep(0.1);
			timeout--;

			if(timeout == 0) {
				sock[socket].state = ESTABLISHED; //Connexion établie par défaut
			}
		}
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu PDU_SYN; //PDU envoyé pour la demande de connection (SYN)
    mic_tcp_pdu* PDU_SYN_ACK = malloc(sizeof(mic_tcp_pdu)); // PDU reçu pour la mise en connection de la source (SYN_ACK)
	
    //Allocation de la mémoire pour le PDU en attente de réception (SYN-ACK)
    PDU_SYN_ACK->payload.data = malloc(sizeof(char));
    PDU_SYN_ACK->payload.size = sizeof(char);
	
    //PDU envoyé à la fin
    mic_tcp_pdu PDU_ACK; 
    
    //Etablissement du ratio de perte pour la negociation
    env_admissible_loss_rate = 50;

    //Paramétrage du PDU SYN
    PDU_SYN.header.syn = 1; 
    PDU_ACK.header.ack = 0;
    PDU_SYN.header.source_port = sock[socket].addr.port;
    PDU_SYN.header.dest_port=addr.port;
    PDU_SYN.payload.data = malloc(sizeof(env_admissible_loss_rate));
    PDU_SYN.payload.size = sizeof(env_admissible_loss_rate);
    //Envoie du Loss Rate choisi par le client
    *PDU_SYN.payload.data = env_admissible_loss_rate;
    
    //Envoie du PDU SYN avec les informations
    int res = IP_send(PDU_SYN,addr);
    if(res==-1) return -1; //Erreur lors de l'envoie du PDU
    sock[socket].state = SYN_SENT;
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    
    //Reception du SYN ACK
    int value = IP_recv(PDU_SYN_ACK,addr_rcv,1);
    //printf("On a fait IP_recv value : %d syn : %d ack : %d\n",value,PDU_SYN_ACK->header.syn,PDU_SYN_ACK->header.ack  );
    //On entre si on a pas reçu le PDU SYN-ACK comme attendu et on renvoie le SYN
    while(value == -1 || (PDU_SYN_ACK->header.syn != 1 || PDU_SYN_ACK->header.ack != 1)) {
      int res = IP_send(PDU_SYN,addr);
			printf("renvoie du SYN car SYN ACK non reçu\n");
			if(res==-1) return -1; //Erreur lors de l'envoie du PDU
			sock[socket].state = SYN_SENT;
      value = IP_recv(PDU_SYN_ACK,addr_rcv,1) ;
      printf("SYN ACK : %d", value);
						
    }

    //Lecture du Loss rate final décidé par le serveur
    rec_admissible_loss_rate = *PDU_SYN_ACK->payload.data;
    printf("Le loss rate final est : %d\n",rec_admissible_loss_rate);
	
    //Paramétrage du PDU ACK envoyé pour finaliser la connexion
    PDU_ACK.header.ack = 1;
    PDU_ACK.header.syn = 0;
    PDU_ACK.header.source_port = sock[socket].addr.port;
    PDU_ACK.header.dest_port=addr.port;
    PDU_ACK.payload.data = malloc(sizeof(rec_admissible_loss_rate));
    PDU_ACK.payload.size = sizeof(rec_admissible_loss_rate);
    //Confirmation du choix du Loss rate
    *PDU_ACK.payload.data = rec_admissible_loss_rate;
		
    //Envoi du PDU ACK	
    res = IP_send(PDU_ACK,addr);
    if(res==-1) return -1; //Erreur lors de l'envoie du PDU
		
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    mic_tcp_pdu PDU_MSG; //PDU de la donnée envoyé
    mic_tcp_pdu* PDU_ACK = malloc(sizeof(mic_tcp_pdu)); //PDU en attente de réception
    PDU_ACK->payload.data = malloc(sizeof(char));
    PDU_ACK->payload.size = sizeof(char);
    mic_tcp_sock_addr addr;

    addr.port= 1234;
    
    //Paramétrage du PDU de la donnée envoyé
    PDU_MSG.header.source_port=sock[mic_sock].addr.port; 
    PDU_MSG.header.dest_port=addr.port; 
    PDU_MSG.payload.data = mesg;
    PDU_MSG.payload.size = mesg_size;
    PDU_MSG.header.ack = 0;
    PDU_MSG.header.syn = 0;
    PDU_MSG.header.fin = 0;
    PDU_MSG.header.seq_num = num_seq;
    
    //Envoie de la donnée
    int res; //= IP_send(PDU_MSG,addr);
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    
    int value; //= IP_recv(PDU_ACK,addr_rcv,100);
    int admissible = 0;
    int random;
    //On répète l'envoie tant qu'on a pas reçu d'aquittement (problème d'envoi, de réception d'acquittement ou de numéro de séquence désynchronisé) 
    //On sort également de la boucle si l'erreur n'est pas prise en compte via le Loss Rate
    do {
	res = IP_send(PDU_MSG,addr);
	if (res==-1) return -1; //erreur lors de la transmission
	value = IP_recv(PDU_ACK,addr_rcv,10);
	    
	//Choix de la prise en compte de la donnée perdu (pas d'acquittement reçu) ou non
        random=rand()%100;
	if(value==-1 &&  random<rec_admissible_loss_rate) {
		printf("On ne prend pas cette erreur en compte \n");
		admissible = 1;
		break;
	}

	printf("Message pas reçu, renvoie du PDU n°ack reçu : value : %d  ack reçu : %d, ack : %d, h_ack : %d\n",value, PDU_ACK->header.ack_num, num_ack, PDU_ACK->header.ack);
    } while(value == -1  || (PDU_ACK->header.ack_num!=num_ack && PDU_ACK->header.ack != 1)); 
    
    //Si on ne prend pas en compte la donnée perdu, on ne touche pas au numéro de séquence et d'acquittement
    if(admissible == 0) {
	num_seq = num_seq+1;
	printf("-------------------------------------------------numero sequence---------------------------------------------- : %d\n",num_seq);
	num_ack +=1;
    }
    
    return 0;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    mic_tcp_payload data;
    data.size = max_mesg_size;
    data.data = mesg;
    int data_size = app_buffer_get(data);
    printf("La data reçu par recv est %s\n",data.data);

    return data_size;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    mic_tcp_pdu PDU_FIN;
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    sock[socket].state = CLOSED;
    PDU_FIN.header.fin = 1;
    IP_send(PDU_FIN,sock[socket].addr);
    return -1;
}
/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
   //Réception du PDU SYN (connection entrante)
   if(pdu.header.syn == 1) {
	sock[0].state = SYN_RECEIVED;
	rec_admissible_loss_rate = *pdu.payload.data;
	printf("J'ai reçu le loss rate : %d\n",rec_admissible_loss_rate);
   }

   //Réception du PDU ACK (acquittement)
   if(pdu.header.ack == 1 && pdu.header.syn != 1) {
   	sock[0].state = ESTABLISHED;
   }

   //Reception d'un message, numéro de séquence correct : envoie d'un PDU d'acquittement
   if(pdu.header.syn == 0 && pdu.header.ack ==0 && pdu.header.fin == 0 && pdu.header.seq_num == num_seq) {
	num_seq = num_seq+1;
	printf("-------------------------------------------------numero sequence---------------------------------------------- : %d\n",num_seq);
	app_buffer_put(pdu.payload);
			
	struct mic_tcp_pdu PDU_ACK;
       //Envoie acquitement du message
      	PDU_ACK.header.ack = 1;
      	PDU_ACK.header.syn = 0;
      	PDU_ACK.header.source_port = pdu.header.dest_port;
      	PDU_ACK.header.dest_port=addr.port;
      	PDU_ACK.payload.data = malloc(sizeof(char));
      	PDU_ACK.payload.size = sizeof(char);
	PDU_ACK.header.ack_num =num_ack;
      	num_ack += 1;
      	//printf("on va envoyer ack\n");
      	int res = IP_send(PDU_ACK,addr);

   }
    //Renvoie Acquitement du message en cas de perte (numéro de séquence désynchronisé)
   else if(pdu.header.seq_num != num_seq && pdu.header.syn == 0 && pdu.header.ack ==0 && pdu.header.fin == 0) {
      	struct mic_tcp_pdu PDU_ACK;
      	printf("-------------------------------------------------numero sequence---------------------------------------------- : %d\n",num_seq);
	app_buffer_put(pdu.payload);
      	PDU_ACK.header.ack = 1;
      	PDU_ACK.header.syn = 0;
      	PDU_ACK.header.source_port = pdu.header.dest_port;
      	PDU_ACK.header.dest_port=addr.port;
      	PDU_ACK.payload.data = malloc(sizeof(char));
      	PDU_ACK.payload.size = sizeof(char);
	PDU_ACK.header.ack_num =abs(pdu.header.seq_num - num_seq);
      	printf("on va reenvoyer ack\n");
      	int res = IP_send(PDU_ACK,addr);
   }		
}


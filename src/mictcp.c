#include <mictcp.h>
#include <api/mictcp_core.h>

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
 
mic_tcp_sock sock[10];
int num_sock = 0;
 
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
   
   set_loss_rate(0);

   return num_sock-1;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
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

		while(sock[socket].state != SYN_RECEIVED){
			sleep(0.1);
		}
    
    mic_tcp_pdu PDU_SYN_ACK; //PDU envoyé (SYN-ACK) pour informé de la mise en connection
    mic_tcp_pdu* PDU_ACK = malloc(sizeof(mic_tcp_pdu)); //PDU en attente de réception (ACK) pour informé de la connection de l'hôte
		PDU_ACK->payload.data = malloc(sizeof(char));
    PDU_ACK->payload.size = sizeof(char);

		
    PDU_SYN_ACK.header.syn=1; //SYN = 1
    PDU_SYN_ACK.header.ack=1; //ACK = 1
    PDU_SYN_ACK.header.source_port=sock[socket].addr.port; //Port source
    PDU_SYN_ACK.header.dest_port=addr->port; //Port destination
		PDU_SYN_ACK.payload.data = malloc(sizeof(char));
		PDU_SYN_ACK.payload.size = sizeof(char);
        

    int res= IP_send(PDU_SYN_ACK,*addr);
    if (res==-1)  return -1; //Problème lors de l'envoie du PDU
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    

    while(sock[socket].state != ESTABLISHED) {
			sleep(0.1);
		}
		printf("ACK reçu\n");
		/*printf("On va recevoir le ACK\n");
    printf("test pdu_ack : %d \n",PDU_ACK->header.ack);
    if(value != -1) {
				printf("On est rentré (truc reçu)\n");
        if(PDU_ACK->header.ack == 1) {
            printf("connection établie \n"); //On a reçu un ACK
            return 0;
        }
    }
		printf("ACK pas reçu\n");*/
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
		PDU_SYN_ACK->payload.data = malloc(sizeof(char));
		PDU_SYN_ACK->payload.size = sizeof(char);
        mic_tcp_pdu PDU_ACK; //PDU envoyé à la fin
    
    //Paramétrage du PDU SYN
    PDU_SYN.header.syn=1; 
		PDU_ACK.header.ack = 0;
    PDU_SYN.header.source_port = sock[socket].addr.port;
    PDU_SYN.header.dest_port=addr.port;
	  PDU_SYN.payload.data = malloc(sizeof(char));
	  PDU_SYN.payload.size = sizeof(char);
    
    
    int res = IP_send(PDU_SYN,addr);
		printf("On a fait IP_SEND \n");
    if(res==-1) return -1; //Erreur lors de l'envoie du PDU
    sock[socket].state = SYN_SENT;
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    
    int value = IP_recv(PDU_SYN_ACK,addr_rcv,1);
    printf("On a fait IP_recv \n");
    if(value != -1) {
        if(PDU_SYN_ACK->header.syn == 1 && PDU_SYN_ACK->header.ack == 1) {
            printf("connection établie \n"); //On a reçu un SYN-ACK
            
						PDU_ACK.header.ack = 1;
						PDU_ACK.header.syn = 0;
						PDU_ACK.header.source_port = sock[socket].addr.port;
						PDU_ACK.header.dest_port=addr.port;
						PDU_ACK.payload.data = malloc(sizeof(char));
	  				PDU_ACK.payload.size = sizeof(char);
						printf("On va envoyer le ACK\n");
						
						int res = IP_send(PDU_ACK,addr);
						printf("apres IPsend de ACK\n");
    				if(res==-1) return -1; //Erreur lors de l'envoie du PDU

						return 0;
        }
    }
    
    sock[socket].state = IDLE;
    
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

		addr.port= 10000;
    
    //Paramétrage du PDU de la donnée envoyé
    PDU_MSG.header.source_port=sock[mic_sock].addr.port; 
    PDU_MSG.header.dest_port=addr.port; 
    PDU_MSG.payload.data = mesg;
    PDU_MSG.payload.size = mesg_size;
		PDU_MSG.header.ack = 0;
		PDU_MSG.header.syn = 0;
		PDU_MSG.header.fin = 0;
    
    //Envoie de la donnée
    int res = IP_send(PDU_MSG,addr);
    printf("apres send PDU_MSG \n");
    if (res==-1) return -1; //erreur lors de la transmission
    
    mic_tcp_sock_addr* addr_rcv = malloc(sizeof(mic_tcp_sock_addr));
    
    int value = IP_recv(PDU_ACK,addr_rcv,1);
    printf("apres recv PDU_ACK \n");
    if(value != -1) {
        if(PDU_ACK->header.ack == 1) {
            printf("Bonne reception de la donnée envoyée \n"); //On a reçu un ACK
            return 0;
        }
    }

    
    return -1;
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
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
		printf("Reception d'un PDU : SYN = %d; ACK = %d\n",pdu.header.syn,pdu.header.ack);
		if(pdu.header.syn == 1) {
			sock[0].state = SYN_RECEIVED;
		}

		if(pdu.header.ack == 1) {
			sock[0].state = ESTABLISHED;
		}
	
		if(pdu.header.syn == 0 && pdu.header.ack ==0 && pdu.header.fin == 0) {
			printf("La data reçu par process est %s\n",pdu.payload.data);
    	app_buffer_put(pdu.payload);
		}

	
}

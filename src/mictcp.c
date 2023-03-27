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
   sock[0].fd = num_sock;
   sock[0].state = IDLE;
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
   
   if(num_sock<socket){
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
    
    mic_tcp_pdu PDU_SYN_ACK; //PDU envoyé (SYN-ACK) pour informé de la mise en connection
    mic_tcp_pdu* PDU_ACK; //PDU en attente de réception (ACK) pour informé de la connection de l'hôte
    
    PDU_SYN_ACK.header.syn=1; //SYN = 1
    PDU_SYN_ACK.header.ack=1; //ACK = 1
    PDU_SYN_ACK.header.source_port=sock[socket].addr.port; //Port source
    PDU_SYN_ACK.header.dest_port=addr->port; //Port destination
    
    
    int res= IP_send(PDU_SYN_ACK,mic_tcp_sock_addr);
    if res==-1  return -1; //Problème lors de l'envoie du PDU
    
    mic_tcp_sock_addr* addr_rcv;
    
    int value = IP_recv(PDU_ACK,addr_rcv,1);
    if(value != -1) {
        if(PDU_ACK->header.ack == 1) {
            printf("connection établie \n"); //On a reçu un ACK
            return 0;
        }
    }
    
    return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu PDU_SYN; //PDU envoyé pour la demande de connection (SYN)
    mic_tcp_pdu* PDU_SYN_ACK; // PDU reçu pour la mise en connection de la source (SYN_ACK)
    
    //Paramétrage du PDU SYN
    PDU_SYN.header.syn=1; 
    PDU_SYN.header.source_port = sock[socket].addr.port;
    PDU_SYN.header.dest_port=addr.port;
    
    
    int res = IP_send(PDU_SYN,mic_tcp_sock_addr);
    if(res==-1) return -1; //Erreur lors de l'envoie du PDU
    sock[socket].state = SYN_SENT;
    
    mic_tcp_sock_addr* addr_rcv;
    
    int value = IP_recv(PDU_SYN_ACK,addr_rcv,1);
    if(value != -1) {
        if(PDU_SYN_ACK->header.syn == 1 && PDU_SYN_ACK->header.ack == 1) {
            printf("connection établie \n"); //On a reçu un SYN-ACK
            sock[socket].state = ESTABLISHED;
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
    mic_tcp_pdu* PDU_ACK; //PDU en attente de réception
    
    //Paramétrage du PDU de la donnée envoyé
    PDU_MSG.header.source_port=sock[mic_sock].addr.port; 
    PDU_MSG.header.dest_port=addr.port; 
    PDU_MSG.payload.data = mesg;
    PDU_MSG.payload.size = mesg_size;
    
    //Envoie de la donnée
    int res = IP_send(PDU_MSG,mic_tcp_sock_addr);
    if (res==-1) return -1; //erreur lors de la transmission
    
    mic_tcp_sock_addr* addr_rcv;
    
    int value = IP_recv(PDU_ACK,addr_rcv,1);
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
    data.size = mex_mesg_size;
    data.data = mesg;
    int data_size = app_buffer_get(data);

    return data_size;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
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
    app_buffer_put(pdu.payload);
}

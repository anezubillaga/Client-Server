#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

#define LBUFFER  200


typedef struct client{
	int socket;
	FILE *f;
	char id[12];
	char nick[16];
	int contrasena;
	time_t tiempoCli;
	bool conectado;
}Cliente;


void alphanumericseq (int length, char cambio[]);


int main(int argc, char **argv){
	FILE *auxfd, *txt;
	char buffer_fd[LBUFFER], aux[LBUFFER];
	time_t tiempoServ;
	int opt = 1;
	int socketfd, new_socket;
	int i, maxClientes=atoi(argv[2]);
	int max_sd, sd;
	//Set of socket descriptors
	fd_set readfds;
	//Para la contraseña:
	int contrasena, numa, numb, contraux;
	struct sockaddr_in servidor_dir, cliente;
	char idaux[12], id[12], nickinvitado[] = "invitado", nick[16], nickaux[16];
	Cliente *client = malloc(sizeof(Cliente) * maxClientes);

	signal(SIGPIPE, SIG_IGN);

	//Inicializo todos los clientes a 0
	for (i = 0; i < maxClientes; i++){
		client[i].socket=-1;
		client[i].conectado = false;
	}
	system("clear");
	//Para que no genere rand siempre los mismos valores.
	srand (time(NULL));

	if ((socketfd=socket(PF_INET,SOCK_STREAM,0)) == -1) {
		perror("Error de Socket() ");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) <0)
	{
		perror("Setsockopt:" );
		exit(EXIT_FAILURE);
	}
	//DefiniendoServidor->Ip,Puerto
	servidor_dir.sin_family=AF_INET;
	servidor_dir.sin_addr.s_addr=(INADDR_ANY);
	servidor_dir.sin_port=htons(atoi(argv[1]));

	if ((bind(socketfd, (struct sockaddr*)&servidor_dir, sizeof(servidor_dir)))==-1)
	{
		printf("Error en el bind: %s\n", strerror(errno));
		close(socketfd);
		exit(EXIT_FAILURE);
	}
	if ((listen(socketfd, 3)) < 0)
	{
		perror("Listen fallido");
		close(socketfd);
		exit(EXIT_FAILURE);
	}
	int dirlen = sizeof(servidor_dir);
	int numConectados = 0;
	tiempoServ = time(NULL);
	printf("Servidor conectado\n");
	while(1)
	{
		//Inicializar el set de sockets.
		FD_ZERO(&readfds);
		//Añado el socket principal del servidor
		FD_SET(socketfd, &readfds);
		max_sd = socketfd;
		//Añado los sockets de los clientes.
		for (i = 0; i < maxClientes; i++)
		{
			//Socket descriptor
			sd = client[i].socket;
			//Si el socket descriptor es válido, lo añado a la lista
			if (sd != -1)
				FD_SET(sd, &readfds);
				//Actualizo el socket descriptor más alto
			if (sd > max_sd)
				max_sd = sd;
		}
		//Esperar a una actividad en uno de los sockets.
		int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0 ) && (errno!=EINTR))
		{
			perror("Error select: ");
		}
		//Compruebo si tengo peticiones por parte de clientes
		for (i = 0; i < maxClientes; i++)
		{
			sd = client[i].socket;
			if (FD_ISSET(client[i].socket, &readfds))
			{
				//Comprobar si era para cerrar
				if ((fgets(buffer_fd, LBUFFER, client[i].f))==NULL)
				{
					//Alguien se ha desconectado.
					getpeername(sd, (struct sockaddr*)&servidor_dir, (socklen_t*)&dirlen);
					printf("Host con id %s desconectado, ip %s, puerto %d\n", client[i].id, inet_ntoa(servidor_dir.sin_addr), ntohs(servidor_dir.sin_port));
					//Cerrar el socket y marcarlo en la lista de reuso
					close(sd);
					client[i].socket = -1;
					fclose(client[i].f);
					numConectados--;
				}
				else{

					txt = fopen("servidor.txt","r+");
					if(txt == NULL){
						txt = fopen("servidor.txt", "w+");
					}
					//Comprobamos si es LOGIN
					if((strncmp(buffer_fd, "LOGIN %s %d\n", 6)) == 0){
						sscanf(buffer_fd, "LOGIN %s %d\n", id, &contrasena);
						/*Tengo que recorrer el fichero servidor.txt para encontrar que
						existe dicho usuario y la contraseña coincide*/
						while (!feof(txt)&&((strcmp(id, idaux))!=0)){
							fgets(aux, LBUFFER, txt);
							sscanf(aux, "%s %d %s\n", idaux, &contraux, nick);
						}
						//Si ha coincidido:
						if (strcmp(id, idaux)==0){
							strcpy(client[i].id,id);
							strcpy(client[i].nick, nick);
							//Si coincide la contraseña
							if (contrasena == contraux){
								client[i].contrasena = contrasena;
								printf("Usuario con id %s aceptado\n", id);
								fprintf(client[i].f, "LOGIN OK\n");
								fflush(client[i].f);
								client[i].conectado = true;
							}
							else{
								printf("Contraseña mal introducida\n");
								fprintf(client[i].f, "LOGIN ERROR\n");
								numConectados--;
								fflush(client[i].f);
								fclose(client[i].f);
								client[i].socket = -1;
								close(client[i].socket);
							}
						}
						else{
							printf("Usuario no encontrado\n");
							fprintf(client[i].f, "LOGIN ERROR\n");
							numConectados--;
							fflush(client[i].f);
							fclose(client[i].f);
							client[i].socket = -1;
							close(client[i].socket);
						}
						fclose(txt);
					}else if ((strcmp(buffer_fd, "REGISTRAR\n")) == 0){
						numa = rand() % 10;
						numb = rand() % 10;
						client[i].contrasena = numa+numb;
						printf("Recibida petición de registro\n");
						printf("Estableciendo prueba %d + %d. ", numa, numb);
						fprintf(client[i].f, "RESUELVE %d %d\n", numa, numb);
						fflush(client[i].f);
					}else if ((strncmp(buffer_fd, "RESPUESTA %d\n", 9))==0)
					{
						sscanf(buffer_fd, "RESPUESTA %d\n", &contrasena);
						if (client[i].contrasena != contrasena){
							printf("Error de registro\n");
							fprintf(client[i].f, "REGISTRADO ERROR\n");
							fflush(client[i].f);
							fclose(client[i].f);
							numConectados--;
							close(client[i].socket);
							client[i].socket = -1;
							fclose(txt);
						}
						else {
							printf("Recibido %d, prueba superada.\n", contrasena);
							//Para que salga desde 6 hasta 12.
							int length = rand() % 6;
							length+=6;
							//Donde almaceno el id del cliente.
							alphanumericseq(length, id);
							while(!feof(txt))
							{
								fgets(aux, LBUFFER, txt);
								sscanf(aux, "%s %d\n", idaux, &contraux);
								//Si en el fichero ya existe un id igual, lo reasignamos.
								if ((strcmp(id, idaux)) == 0)
								{
									alphanumericseq(length, id);
									fclose(txt);
									txt = fopen("servidor.txt","r");
								}
							}
							printf("Asignando id %s\n", id);
							strcpy(client[i].id, id);
							fprintf(client[i].f, "REGISTRADO OK %s\n", id);
							fflush(client[i].f);
							client[i].conectado = true;
							fclose(txt);
							txt = fopen("servidor.txt", "a");
							strcpy(client[i].nick, nickinvitado);
							fprintf(txt, "%s %d %s\n", id, contrasena, nickinvitado);
							fclose(txt);
							}
					}
					else if ((strncmp(buffer_fd, "SETNAME %s\n", 8)) == 0 && (client[i].conectado == true))
					{
						//vacio nick.
						memset(nick, 0, sizeof(nick));
						sscanf(buffer_fd, "SETNAME %s\n", nick);
						auxfd = fopen("aux.txt", "w");
						rewind(txt);
						while (!feof(txt))
						{
							fgets(aux, LBUFFER, txt);
							sscanf(aux, "%s %d %s\n", idaux, &contraux, nickaux);
							if (strcmp(client[i].id, idaux)!=0 && !feof(txt)){
								fprintf(auxfd, "%s %d %s\n", idaux, contraux, nickaux);
							}
						}
						fprintf(auxfd, "%s %d %s\n", client[i].id, client[i].contrasena, nick);
						fflush(auxfd);
						fflush(txt);
						fclose(auxfd);
						fclose(txt);
						if (remove("servidor.txt")<0)
						{
							fprintf(client[i].f, "SETNAME ERROR\n");
							fflush(client[i].f);
						}
						if (rename("aux.txt","servidor.txt")<0)
						{
							fprintf(client[i].f, "SETNAME ERROR\n");
							fflush(client[i].f);
						}
						else{
							strcpy(client[i].nick,nick);
							printf("Usuario con id %s ha cambiado su nombre a %s\n", client[i].id, nick);
							fprintf(client[i].f, "SETNAME OK\n");
							fflush(client[i].f);
						}
					}
					else if ((strcmp(buffer_fd, "GETNAME\n")) == 0 && (client[i].conectado == true))
					{
						printf("El nick es %s\n", client[i].nick);
						fprintf(client[i].f, "GETNAME %s\n", client[i].nick);
						fflush(client[i].f);
					}
					else if ((strcmp(buffer_fd, "CERRAR\n"))==0 && (client[i].conectado == true))
					{
						printf("Conexión con %s interrumpida\n", client[i].id);
						numConectados--;
						fclose(client[i].f);
						client[i].socket = -1;
						close(client[i].socket);
						fclose(txt);
					}
					else if ((strcmp(buffer_fd, "LISTA\n"))==0 && (client[i].conectado == true))
					{
						char conectados[maxClientes*16];
						memset(conectados, 0, sizeof(conectados));
						for (int h = 0; h<maxClientes; h++)
						{
							if (client[h].socket!=-1)
							{
								printf("El cliente %d está conectado con nick %s\n", h, client[h].nick);
								strcat(conectados, " ");
								strcat(conectados, client[h].nick);
							}
						}
						printf("Conectados: %s\n", conectados);
						fprintf(client[i].f, "LISTADO %s\n", conectados);
						fflush(client[i].f);
					}
					else if ((strcmp(buffer_fd, "UPTIME\n"))==0 && (client[i].conectado == true))
					{
						time_t actual = time(NULL);
						printf("Enviando información de tiempo a %s\n", client[i].id);
						fprintf(client[i].f, "UPTIME %d %d\n", (int)difftime(actual, tiempoServ), (int)difftime(actual, client[i].tiempoCli));
						fflush(client[i].f);
					}
					else{
						printf("No se ha recibido ninguna petición\n");
						fclose(client[i].f);
						close(client[i].socket);
						client[i].socket = -1;
						numConectados--;
						fclose(txt);
					}
				}
			}
		}
		//Si se detecta algo proveniente del socket del servidor, significa que hay clientes intentando conectarse
		if (FD_ISSET(socketfd, &readfds))
		{
			new_socket = 0;
			bool maximo = true;
			for(int h = 0; h < maxClientes; h++){
				if (client[h].socket==-1)
					maximo = false;
			}
			if (maximo == true)
			{
				printf("Conexión rechazada\n");
			}
			else if ((new_socket = accept(socketfd, (struct sockaddr *) &cliente, (socklen_t*)&dirlen))<0)
			{
				perror("Error en accept: ");
				exit(EXIT_FAILURE);
			}
			if (new_socket > 0)
			{
				//Un cliente se ha conectado, así que informamos de ello
				printf("Nueva conexión: socket fd %d, ip %s, puerto %d\n", new_socket, inet_ntoa(cliente.sin_addr), ntohs(servidor_dir.sin_port));
				//Añado el nuevo socket al array de clientes
				for (i = 0; i<maxClientes; i++)
				{
					//Si la posición está vacía
					if (client[i].socket == -1)
					{
						numConectados++;
						client[i].tiempoCli = time(NULL);
						client[i].socket = new_socket;
						client[i].f = fdopen(client[i].socket,"rw+"); //Añadir el reloj para el cliente.
						break;
					}
				}
				printf("Número de clientes conectados: %d\n", numConectados);
			}
		}
	}
	close(socketfd);
	return EXIT_SUCCESS;
}


void alphanumericseq(int length, char cambio[]){
	char seq[] = "qwertyuiopasdfghjklzxcvbnm1234567890";
	for (int i = 0; i < length - 1 ; i++)
	{
		cambio[i] = seq[rand() % (sizeof(seq) - 1)];
	}
	cambio[length - 1] = '\0';
}

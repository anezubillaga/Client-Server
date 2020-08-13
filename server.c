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

#define LBUFFER  200

FILE *f, *auxfd, *txt;
char buffer_fd[LBUFFER], aux[LBUFFER];

void alphanumericseq (int length, char cambio[]);
void nombre(char id[], char nick[]);

int main(int argc, char **argv){
	int socketfd,c_sock;
	//Para la contraseña:
	int contrasena, numa, numb, contraux;
	struct sockaddr_in servidor_dir,cliente;
	char idaux[12], id[12], nickinvitado[] = "invitado", nick[16]; //nickaux[16];
	FILE *txt;

	system("clear");
	//Para que no genere rand siempre los mismos valores.
	srand (time(NULL));

	if ((socketfd=socket(PF_INET,SOCK_STREAM,0)) == -1) {
		perror("Error de Socket() ");
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
	if ((listen(socketfd, 5)) < 0)
	{
		perror("Listen fallido");
		close(socketfd);
		exit(EXIT_FAILURE);
	}
	while(1)
	{
	//Para abrir el fichero o Crearlo en caso de que no exista, y no se sobreescriba .
		printf("Escuchando conexiones en el puerto\n");
		int dirlen = sizeof(servidor_dir);
		if ((c_sock = accept(socketfd,(struct sockaddr*)&cliente,(socklen_t*)&dirlen)) < 0)
		{
			close(socketfd);
			perror("Fallo aceptando la conexión");
			exit(EXIT_FAILURE);
		}

		printf("Conexión establecida desde la IP %s puerto %hu\n", inet_ntoa(cliente.sin_addr), htons(atoi(argv[1])));

		txt = fopen("servidor.txt","r+");
		f = fdopen (c_sock, "w+");
		if (txt == NULL)
		{
			txt = fopen("servidor.txt", "w+");
		}

		fgets(buffer_fd, LBUFFER, f);
		if ((strncmp(buffer_fd, "LOGIN %s %d\n", 6)) == 0)
		{
				sscanf(buffer_fd, "LOGIN %s %d\n", id, &contrasena);
				/*Tengo que recorrer el fichero servidor.txt para encontrar que
				existe dicho usuario y la contraseña coincide*/
				while (!feof(txt)&&((strcmp(id, idaux))!=0))
				{
					fgets(aux, LBUFFER, txt);
					sscanf(aux, "%s %d %s\n", idaux, &contraux, nick);
				}
				//Si ha coincidido:
				if (strcmp(id, idaux)==0)
				{
					//Si coincide la contraseña
					if (contrasena == contraux)
					{
						printf("Usuario con id %s aceptado\n", id);
						fprintf(f, "LOGIN OK\n");
						//Si se ha conseguido login, se hace los cambios / get name
						nombre(id,nick);
					}
					else
					{
						printf("Contraseña mal introducida\n");
						fprintf(f, "LOGIN ERROR\n");
						fclose(f);
					}
				}
				else
				{
					printf("Usuario no encontrado\n");
					fprintf(f, "LOGIN ERROR\n");
					fclose(f);
				}
				fclose(txt);
		}
		else if ((strcmp(buffer_fd, "REGISTRAR\n")) == 0)
		{
			numa = rand() % 10;
			numb = rand() % 10;
			printf("Recibida petición de registro\n");
			printf("Estableciendo prueba %d + %d. ", numa, numb);
			fprintf(f, "RESUELVE %d %d\n", numa, numb);
			//Ahora compruebo que el resultado es el correcto
			fgets(buffer_fd, LBUFFER, f);
			//Interpreto:
			sscanf(buffer_fd, "RESPUESTA %d\n", &contrasena);
			if (numa + numb != contrasena)
			{
				printf("Error de registro\n");
				fprintf(f, "REGISTRADO ERROR\n");
				close(socketfd);
				fclose(f);
				fclose(txt);
				exit(EXIT_FAILURE);
			}
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
			fprintf(f, "REGISTRADO OK %s\n", id);
			fclose(txt);
			txt = fopen("servidor.txt", "a");
			fprintf(txt, "%s %d %s\n", id, contrasena, nickinvitado);
			fflush(f);
			fclose(txt);
			//Hago el setname / getname
			nombre(id, nickinvitado);

		}
		else
		{
			printf("No se ha recibido ninguna petición\n");
			fclose(f);
			close(socketfd);
			fclose(txt);
			exit(EXIT_FAILURE);
		}
	}
	close(socketfd);
	return EXIT_SUCCESS;
}

void nombre(char id[], char nick[]){
	bool conexion = true;
	while (conexion)
	{
		char nickaux[16], idaux[12];
		int contraux;

		txt = fopen("servidor.txt","r+");
		fgets(buffer_fd, LBUFFER, f);
		if ((strncmp(buffer_fd, "SETNAME %s\n", 8)) == 0)
		{
			sscanf(buffer_fd, "SETNAME %s\n", nick);
			auxfd = fopen("aux.txt", "w");
			rewind(txt);
			while (!feof(txt))
			{
				fgets(aux, LBUFFER, txt);
				sscanf(aux, "%s %d %s\n", idaux, &contraux, nickaux);
				if (strcmp(id, idaux)!=0){
					fprintf(auxfd, "%s %d %s\n", idaux, contraux, nickaux);
				}
			}
			fprintf(auxfd, "%s %d %s\n", id, contraux, nick);
			fflush(auxfd);
			fflush(txt);
			fclose(auxfd);
			fclose(txt);
			if (remove("servidor.txt")<0)
			{
				fprintf(f, "SETNAME ERROR\n");
				fflush(f);
			}
			if (rename("aux.txt","servidor.txt")<0)
			{
				fprintf(f, "SETNAME ERROR\n");
				fflush(f);
			}
			else{
				printf("Usuario con id %s ha cambiado su nombre a %s\n", id, nick);
				fprintf(f, "SETNAME OK\n");
				fflush(f);
			}
		}
		else if ((strcmp(buffer_fd, "GETNAME\n")) == 0)
		{
			fprintf(f, "GETNAME %s\n", nick);
			fflush(f);
		}
		else if ((strcmp(buffer_fd, "CERRAR\n"))==0)
		{
			printf("Conexión con %s interrumpida\n", id);
			conexion = false;
		}
	}
}

void alphanumericseq(int length, char cambio[]){
	char seq[] = "qwertyuiopasdfghjklzxcvbnm1234567890";
	for (int i = 0; i < length - 1 ; i++)
	{
		cambio[i] = seq[rand() % (sizeof(seq) - 1)];
	}
	cambio[length - 1] = '\0';
}

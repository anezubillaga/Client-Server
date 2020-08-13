//Sergio Artieda y Ane Zubillaga

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LBUFFER 2000


FILE *f, *clientefd;
char buffer_fd[LBUFFER];

void nombre();

int main(int argc, char *argv[]){
	int socketfd;
	//Para las contraseñas
	int numa, numb, contrasena;
	char id[12], aux[LBUFFER];
	struct sockaddr_in servidor_dir;
	if (argc != 3)
	{
		printf("Hay que introducir 3 argumentos\n");
		exit(EXIT_FAILURE);
	}

	system("clear");

	// Establecimiento Socket
	if ((socketfd=socket(PF_INET,SOCK_STREAM,0)) == -1) {
		perror("Error de Socket() ");
		exit(EXIT_FAILURE);
	}

	//DefiniendoServidor->Ip,Puerto
	servidor_dir.sin_family=AF_INET;
	servidor_dir.sin_port=htons(atoi(argv[2]));
	servidor_dir.sin_addr.s_addr=inet_addr(argv[1]);

	//Estableciendo Conexion
	if((connect(socketfd, (struct sockaddr *) &servidor_dir,sizeof (struct sockaddr_in))) == -1)
	{
		perror("Error de Connect() ");
		exit(EXIT_FAILURE);
	}
	//Genero el nombre del fichero de forma aleatoria en función de su padre:
	srand(getppid());
	int aleatorio = rand() % 100; //Entre 0 y 100.
	char namepath[3];
	sprintf(namepath, "%d", aleatorio);
	//Abro el fichero de cliente.txt en modo lectura
	clientefd = fopen(namepath, "r");
	//Abrimos el socket en forma de file
	f = fdopen(socketfd, "w+");
	if (f == NULL)
	{
		perror("Error abriendo el fichero socket");
		close(socketfd);
		fclose(f);
		exit(EXIT_FAILURE);
	}

	//Si el fichero ya existe, significa que el usuario ya está registrado. Feof
	//Lo uso para asegurarnos de que existe y además hay contenido.
	if (clientefd != NULL && !feof(clientefd))
	{
		fgets(aux, LBUFFER, clientefd);
		sscanf(aux, "%s %d\n", id, &contrasena);
		printf("Hay datos para el usuario %s, probamos autentificación\n", id);
		fprintf(f, "LOGIN %s %d\n", id, contrasena);
		fgets(buffer_fd, LBUFFER, f);
		if ((strcmp(buffer_fd, "LOGIN OK\n"))==0)
		{
			printf("Sesión establecida con id %s\n", id);
			fclose(clientefd);
			while(1)
			{
				nombre();
			}
		}
		else if ((strcmp(buffer_fd, "LOGIN ERROR\n"))==0)
		{
			printf("No se pudo establecer la sesión\n");
			fclose(clientefd);
		}
		else
		{
			printf("ERROR\n");
			fclose(clientefd);
		}
	}
	else
	{
		//Se registra
		fprintf(f, "REGISTRAR\n");
		//Con sscanf, analiza el mensaje del buffer y almacenamos lo que queremos
		fgets(buffer_fd, LBUFFER, f);
		sscanf(buffer_fd, "RESUELVE %d %d\n", &numa, &numb);
		printf("Resuelve %d + %d = %d", numa, numb, (numa+numb));

		fprintf(f, "RESPUESTA %d\n", (numa+numb));
		//Cojo mi id:
		fgets(buffer_fd, LBUFFER, f);
		if ((strcmp(buffer_fd, "REGISTRADO ERROR\n"))==0)
		{
			printf("No superada la prueba de registro\n");
		}
		else if ((strncmp(buffer_fd, "REGISTRADO OK %s\n", 13))==0)
		{
			clientefd = fopen(namepath, "w");
			sscanf(buffer_fd, "REGISTRADO OK %s\n", id);
			printf("\nSesión establecida con id %s\n", id);
			printf("Conexión abierta\n");
			fprintf(clientefd, "%s %d\n", id, (numa+numb));
			fflush(clientefd);
			while(1)
			{
				nombre();
			}
		}
		else
		{
			printf("No superada la prueba de registro\n");
			/*close(socketfd);
			fclose(f);*/
		}
	}
	return EXIT_SUCCESS;
}

void nombre(){
	int opcion;
	char nickname[LBUFFER];
	printf("¿Qué quieres hacer?\n");
	printf("1. Cambiar nombre\n");
	printf("2. Ver tu nombre\n");
	printf("3. Ver qué usuarios hay conectados\n"); //Los diferentes de menos 1.
	printf("4. Tiempo que lleva el servidor funcionando\n");
	printf("5. Tiempo que llevamos conectados\n");
	printf("6. Cerrar conexión\n");
	printf("Opción: ");
	scanf(" %d", &opcion);
	if (opcion == 1)
	{
		printf("Elige nombre: ");
		scanf(" %s", nickname);
		while (strlen(nickname)>16)
		{
			printf("El tamaño del nombre debe de ser menor que 16 caracteres, vuelve a introducirlo:\n");
			scanf(" %s", nickname);
		}
		fprintf(f, "SETNAME %s\n", nickname);
		fgets(buffer_fd, LBUFFER, f);
		if ((strcmp(buffer_fd, "SETNAME OK\n"))==0)
		{
			printf("Nombre cambiado con éxito\n");
		}
		else if ((strcmp(buffer_fd, "SETNAME ERROR\n"))==0)
		{
			printf("Error cambiando el nombre\n");
		}
	}
	else if (opcion == 2)
	{
		fprintf(f, "GETNAME\n");
		fgets(buffer_fd, LBUFFER, f);
		sscanf(buffer_fd, "GETNAME %s\n", nickname);
		printf("El nickname es %s\n", nickname);
	}
	else if (opcion == 3)
	{
		fprintf(f, "LISTA\n");
		fgets(buffer_fd, LBUFFER, f);
		printf("%s", buffer_fd);
	}
	else if (opcion == 4)
	{
		long tiemposerv, tiempoyo;
		fprintf(f, "UPTIME\n");
		fgets(buffer_fd, LBUFFER, f);
		sscanf(buffer_fd, "UPTIME %ld %ld\n", &tiemposerv, &tiempoyo);
		printf("El servidor lleva %ld segundos funcionando\n", tiemposerv);
	}
	else if (opcion == 5)
	{
		long tiemposerv, tiempoyo;
		fprintf(f, "UPTIME\n");
		fgets(buffer_fd, LBUFFER, f);
		sscanf(buffer_fd, "UPTIME %ld %ld\n", &tiemposerv, &tiempoyo);
		printf("LLevo %ld segundos conectado\n", tiempoyo);
	}
	else if (opcion == 6)
	{
		fprintf(f, "CERRAR\n");
		fclose(f);
		fclose(clientefd);
		exit(EXIT_SUCCESS);
	}
	else
	{
		printf("Por favor escoja una opción válida\n");
	}
}

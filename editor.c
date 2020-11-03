#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MASTER 0

/* Structura care defineste un pixel: in cazul unei poze color, folosim primele 3
variabile (r, g, b), altfel daca e alb-negru folosim doar ultima variabila (bw). */
typedef struct {
	unsigned char r, g, b;
	unsigned char bw;
}pixel;

/* Structura care defineste o imagine. Se retine daca imaginea este sau nu color,
maxval, latimea si inaltimea, precum si matricea de pixeli. */
typedef struct {
	unsigned char color;
	unsigned char maxval;
	unsigned int width, height;
	pixel **matrix;
}image;

/* Matrice 3x3 reprezentand filtrul smooth */
void smoothMatrix (float matrix[3][3]) {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			matrix[i][j] = 1.0f / 9;
		}
	}
}

/* Matrice 3x3 reprezentand filtrul blur */
void blurMatrix (float matrix[3][3]) {
	for (int i = 0; i < 3; (i = i + 2)) {
		for (int j = 0; j < 3; (j = j + 2)) {
			matrix[i][j] = 1.0f / 16;
		}
	}
	for (int i = 0; i < 3; ++i) {
		if (i != 1) {
			matrix[i][1] = 2 * (1.0f / 16);
		} else {
			matrix[i][0] = 2 * (1.0f / 16);
			matrix[i][2] = 2 * (1.0f / 16);
		}
	}
	matrix[1][1] = 4 * (1.0f / 16);
}

/* Matrice 3x3 reprezentand filtrul sharpen */
void sharpenMatrix (float matrix[3][3]) {
	for (int i = 0; i < 3; (i = i + 2)) {
		for (int j = 0; j < 3; (j = j + 2)) {
			matrix[i][j] = 0;
		}
	}
	for (int i = 0; i < 3; ++i) {
		if (i != 1) {
			matrix[i][1] = (-2) * (1.0f / 3);
		} else {
			matrix[i][0] = (-2) * (1.0f / 3);
			matrix[i][2] = (-2) * (1.0f / 3);
		}
	}
	matrix[1][1] = 11 * (1.0f / 3);
}

/* Matrice 3x3 reprezentand filtrul mean */
void meanMatrix (float matrix[3][3]) {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (i == 1 && j == 1) {
				matrix[i][j] = 9;
			} else {
				matrix[i][j] = -1;
			}
		}
	}
}

/* Matrice 3x3 reprezentand filtrul emboss */
void embossMatrix (float matrix[3][3]) {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			matrix[i][j] = 0;
		}
	}
	matrix[2][1] = -1;
	matrix[0][1] = 1;
}

/* Functie care se ocupa de clamparea pixelilor la calcularea filtrului*/
float clamp(float sum) {
	if (sum < 0) {
		sum = 0;
	} else if (sum > 255) {
		sum = 255;
	}
	return sum;
}

/* Functie care citeste datele de intrare */
void readInput(image *img, const char * fileName) {
	FILE* fin = fopen(fileName, "r");

	/* Se citesc datele despre imagine si se adauga in structura, ignorand linia
	comentata din imaginea din input */
	char color_pixel[4];
	char ignore[1024];
	fscanf(fin, "%s\n", color_pixel);
	fgets(ignore, sizeof(ignore), fin);

	if (color_pixel[1] == '5') {
		img->color = 0;
	} else {
		img->color = 1;
	}

	fscanf(fin, "%u %u\n", &(img->width), &(img->height));
	fscanf(fin, "%hhu\n", &(img->maxval));
	/* Se maresc dimensiunile matricii pentru a o putea borda cu 0 */
	img->width = img->width + 2;
	img->height = img->height + 2;

	/* Se aloca memorie pentru matricea care va reprezenta imaginea si se citesc pixelii,
	avand grija ca pentru imaginea color sa citim 3 octeti, iar pentru cea alb-negru,
	un singur octet memorat in variabila bw */
	pixel** matrix = (pixel**) malloc(img->height * sizeof(pixel*));
	for (int i = 0; i < img->height; ++i) {
		matrix[i] = (pixel*) malloc(img->width * sizeof(pixel));
		for (int j = 0; j < img->width; ++j) {
			if (!img->color) {
				if ((i == 0) || (i == img->height - 1) || (j == 0) || (j == img->width - 1)) {
					matrix[i][j].bw = 0;
				} else {
					fread(&(matrix[i][j].bw), sizeof(char), 1, fin);
				}
			} else {
				if ((i == 0) || (i == img->height - 1) || (j == 0) || (j == img->width - 1)) {
					matrix[i][j].r = 0;
					matrix[i][j].g = 0;
					matrix[i][j].b = 0;
				} else {
					fread(&(matrix[i][j]), sizeof(char), 3, fin);
				}
			}
		}
	}

	/* Se salveaza matrcea citita in campul corespunzator din structura */
	img->matrix = (pixel**) matrix;
	fclose(fin);
}

/* Functie care generaeaza fisierul de iesire */
void writeData(image *img, const char * fileName) {
	FILE* fout = fopen(fileName, "w");
	pixel** matrix = (pixel**) img->matrix;
	/* Se scriu datele imaginii in fisier */
	if (!img->color) {
		fprintf(fout, "P5\n");
	} else {
		fprintf(fout, "P6\n");
	}
	fprintf(fout, "%u %u\n", img->width - 2, img->height - 2);
	fprintf(fout, "%hhu\n", img->maxval);

	/* Se scriu octetii reprezentand imaginea in fisier, in functie de
	tipul imaginii */
	for (int i = 1; i < img->height - 1; ++i) {
		for (int j = 1; j < img->width - 1; ++j) {
			if (!img->color) {
				fwrite(&(matrix[i][j].bw), sizeof(char), 1, fout);
			} else {
				fwrite(&(matrix[i][j]), sizeof(char), 3, fout);
			}
		}
	}

	/* Se elibereaza memoria */
	for (int i = 0; i < img->height; ++i)
		free(matrix[i]);
	free(matrix);
	fclose(fout);
}

int main (int argc, char *argv[]) {
	int rank;
	int nProcesses;
	int width, lines, start, end;
	int i, j;
	unsigned char color;
	pixel **helper, **partitioned, **aux;
	image img;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* Procesul cu rank-ul MASTER (0) se ocupa de trimiterea datelor necesare prelucrarii imagini
	la celelalte procese */
	if (rank == MASTER) {
		readInput(&img, argv[1]);
		for (int index = 1; index < nProcesses; ++index) {
			end = (index + 1) * img.height / nProcesses;
			start = index * img.height / nProcesses;
			lines = end - start;  /* numarul de linii din matrice trimis fiecarui proces */
			color = img.color;
			width = img.width;
			MPI_Ssend(&color, 1, MPI_CHAR, index, 1, MPI_COMM_WORLD);
			MPI_Ssend(&width, 1, MPI_INT, index, 1, MPI_COMM_WORLD);
			MPI_Ssend(&lines, 1, MPI_INT, index, 1, MPI_COMM_WORLD);
			for (i = start - 1; i <= end; ++i) {
				if (i == end && index == nProcesses - 1) {
					break;
				} else {
					MPI_Ssend(img.matrix[i], img.width * 4, MPI_CHAR, index, 1, MPI_COMM_WORLD);
				}
			}
		}
		width = img.width;
		color = img.color;
		lines = img.height / nProcesses;

		/* Se aloca memorie pentru matrici de dimensiuni lines + 1, fiind necesare
		cele lines linii prelucrate de 0 plus inca una pentru a se aplica filtrul de 3x3 */
		helper = (pixel**) malloc (sizeof(pixel*) * (lines + 1));
		partitioned = (pixel**) malloc (sizeof(pixel*) * (lines + 1));

		for (i = 0; i <= lines; ++i) {
			if (i == lines && nProcesses == 1) {
				break;
			} else {
				helper[i] = (pixel*) malloc (sizeof(pixel) * width);
				partitioned[i] = (pixel*) malloc (sizeof(pixel) * width);
				j = 0;
				while (j < width) {
					partitioned[i][j] = img.matrix[i][j];
					helper[i][j] = img.matrix[i][j];
					j++;
				}
			}
		}
	} else {
		/* Celelalte procese primesc datele trimise de MASTER */
		MPI_Recv(&color, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&width, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&lines, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

		/* Se aloca memorie ca si la procesul MASTER, insa pentru procesele
		care primesc linii care nu sunt margini, se vor copia si liniile de
		deasupra si dedesupt pentru a aplica filtrul de 3x3 */
		int lines_aux;
		if (nProcesses - 1 != rank) {
			lines_aux = lines + 2;	
		} else {
			lines_aux = lines + 1;
		}
		helper = (pixel**) malloc (sizeof(pixel*) * lines_aux);
		partitioned = (pixel**) malloc (sizeof(pixel*) * lines_aux);
		for (i = 0; i <= lines + 1; ++i) {
			if (i == lines + 1 && rank == nProcesses - 1) {
				break;
			} else {
				helper[i] = (pixel*) malloc (sizeof(pixel) * width);
				partitioned[i] = (pixel*) malloc (sizeof(pixel) * width);
				MPI_Recv(partitioned[i], width * 4, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
				j = 0;
				while (j < width) {
					helper[i][j] = partitioned[i][j];
					j++;
				}
			}
		}
	}

	/* Se initializeaza matricile care reprezinta filtre */
	float smooth[3][3];
	float blur[3][3];
	float sharpen[3][3];
	float mean[3][3];
	float emboss[3][3];
	float *matrix;
	blurMatrix(blur);
	smoothMatrix(smooth);
	meanMatrix(mean);
	sharpenMatrix(sharpen);
	embossMatrix(emboss);

	for (int index = 3; index < argc; ++index) {
		/* Aleg matricea/matricile care ma intereseaza */
		if (0 == strcmp(argv[index], "smooth")) {
			matrix = &smooth[0][0];
		} else if (0 == strcmp(argv[index], "blur")) {
			matrix = &blur[0][0];
		} else if (0 == strcmp(argv[index], "sharpen")) {
			matrix = &sharpen[0][0];
		} else if (0 == strcmp(argv[index], "mean")) {
			matrix = &mean[0][0];
		} else if (0 == strcmp(argv[index], "emboss")) {
			matrix = &emboss[0][0];
		}

		/* Dupa fiecare aplicare, se actualizeaza datele si in procesele vecine */
		if (index > 3 && nProcesses > 1) {
			if (nProcesses - 1 == rank) {
				MPI_Sendrecv(partitioned[1], width * 4, MPI_CHAR, rank - 1, 1, partitioned[0], width * 4,
								MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD, &status);	
			} else if (MASTER == rank) {
				MPI_Sendrecv(partitioned[lines - 1], width * 4, MPI_CHAR, 1, 1, partitioned[lines], width * 4,
								MPI_CHAR, 1, 1, MPI_COMM_WORLD, &status);
			} else {
				MPI_Sendrecv(partitioned[1], width * 4, MPI_CHAR, rank - 1, 1, partitioned[0], width * 4,
								MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD, &status);
				MPI_Sendrecv(partitioned[lines], width * 4, MPI_CHAR, rank + 1, 1, partitioned[lines + 1], width * 4,
								MPI_CHAR, rank + 1, 1, MPI_COMM_WORLD, &status);
			}
		}

		/* Se calculeaza noii pixeli ai imaginii pentru fiecare proces in functie de filtru */
		for (i = 1; i <= lines; ++i) {
			if ( i == lines && rank == MASTER) {
				break;
			}
			if (i == lines && rank == nProcesses - 1) {
				break;
			}
			if (i == lines - 1 && nProcesses == 1) {
				break;
			}
			for (j = 1; j < width - 1; ++j) {
				if (color) {
					float sum_r = 0, sum_g = 0, sum_b = 0;
					for (int k = i - 1; k <= i + 1; ++k) {
						for (int l = j - 1; l <= j + 1; ++l) {
							sum_r += (float)partitioned[k][l].r * (*(matrix + 8 - ((k - i + 1) * 3 + (l - j + 1))));
							sum_g += (float)partitioned[k][l].g * (*(matrix + 8 - ((k - i + 1) * 3 + (l - j + 1))));
							sum_b += (float)partitioned[k][l].b * (*(matrix + 8 - ((k - i + 1) * 3 + (l - j + 1))));
						}
					}
					helper[i][j].r = (unsigned char) clamp(sum_r);
					helper[i][j].g = (unsigned char) clamp(sum_g);
					helper[i][j].b = (unsigned char) clamp(sum_b);
				} else {
					float sum = 0;
					for (int k = i - 1; k <= i + 1; ++k) {
						for (int l = j - 1; l <= j + 1; ++l) {
							sum += (float)partitioned[k][l].bw * (*(matrix + 8 - ((k - i + 1) * 3 + (l - j + 1))));
						}
					}
					helper[i][j].bw = (unsigned char) clamp(sum);
				}
			}
		}

		/* Se interschimba matricile pentru ca la aplicarea urmatorului filtru sa se
		poata folosi matricea partitioned */
		aux = partitioned;
		partitioned = helper;
		helper = aux;
	}

	/* Se pune rezultatul obtinut de la procese in matricea de pixeli (de la procesul
	MASTER se pune in matrice, iar de la celelalte se asteapta sa se primeasca) */
	if (rank == MASTER) {
		for (i = 1; i < lines; ++i) {
			for (j = 1; j < width - 1; ++j) {
				img.matrix[i][j] = partitioned[i][j];
			}
		}
		for (int index = 1; index < nProcesses; ++index) {
			for (i = index * img.height / nProcesses;
					i < (index + 1) * img.height / nProcesses; ++i) {
				if (img.height - 1 == i) {
					break;
				}
				MPI_Recv(img.matrix[i], width * 4, MPI_CHAR, index, 1, MPI_COMM_WORLD, &status);
			}
		}
	} else {
		for (i = 1; i <= lines; ++i) {
			if (i == lines && rank == nProcesses - 1) {
				break;
			} else {
				MPI_Ssend(partitioned[i], width * 4, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
			}
		}
	}

	/* Procesul MASTER scrie datele in fisier */
	if (rank == MASTER) {
		writeData(&img, argv[2]);
	}

	MPI_Finalize();
	return 0;
}
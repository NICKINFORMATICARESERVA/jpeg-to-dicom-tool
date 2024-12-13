#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <jpeglib.h>
#include <dcmtk/dcmdata/dctk.h>

// Função para converter JPEG para DICOM

//Instale as Bibliotecas Necessárias:

//libjpeg: Biblioteca para manipulação de arquivos JPEG.

//DCMTK: Toolkit para manipulação de arquivos DICOM.
void convert_to_dicom(const char *jpeg_file, const char *dicom_file) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *infile = fopen(jpeg_file, "rb");
    if (infile == NULL) {
        fprintf(stderr, "Não foi possível abrir o arquivo %s\n", jpeg_file);
        return;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int row_stride = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    DcmFileFormat fileformat;
    DcmDataset *dataset = fileformat.getDataset();

    // Configurar metadados DICOM
    dataset->putAndInsertString(DCM_PatientName, "Test^Patient");
    dataset->putAndInsertString(DCM_PatientID, "12345");

    // Configurar dados da imagem
    Uint16 rows = cinfo.output_height;
    Uint16 cols = cinfo.output_width;
    Uint16 samples_per_pixel = cinfo.output_components;
    Uint16 bits_allocated = 8;
    Uint16 bits_stored = 8;
    Uint16 high_bit = 7;
    Uint16 pixel_representation = 0;

    dataset->putAndInsertUint16(DCM_Rows, rows);
    dataset->putAndInsertUint16(DCM_Columns, cols);
    dataset->putAndInsertUint16(DCM_SamplesPerPixel, samples_per_pixel);
    dataset->putAndInsertUint16(DCM_BitsAllocated, bits_allocated);
    dataset->putAndInsertUint16(DCM_BitsStored, bits_stored);
    dataset->putAndInsertUint16(DCM_HighBit, high_bit);
    dataset->putAndInsertUint16(DCM_PixelRepresentation, pixel_representation);

    Uint8 *pixel_data = (Uint8 *)malloc(rows * cols * samples_per_pixel);
    Uint8 *p = pixel_data;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(p, buffer[0], row_stride);
        p += row_stride;
    }
    dataset->putAndInsertUint8Array(DCM_PixelData, pixel_data, rows * cols * samples_per_pixel);

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    free(pixel_data);

    fileformat.saveFile(dicom_file, EXS_LittleEndianExplicit);
}

// Função principal
int main() {
    DIR *dir;
    struct dirent *ent;
    char jpeg_path[256];
    char dicom_path[256];

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".jpeg") || strstr(ent->d_name, ".jpg")) {
                snprintf(jpeg_path, sizeof(jpeg_path), "%s", ent->d_name);
                snprintf(dicom_path, sizeof(dicom_path), "%s.dcm", ent->d_name);
                convert_to_dicom(jpeg_path, dicom_path);
            }
        }
        closedir(dir);
    } else {
        perror("Não foi possível abrir o diretório");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

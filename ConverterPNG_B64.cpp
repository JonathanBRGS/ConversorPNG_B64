#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

// Definindo funções que serão globais:

// Função para abrir a caixa de diálogo de seleção de arquivo

bool OpenFileDialog(HWND hWnd, LPWSTR selectedFile)
{
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = selectedFile;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"PNG Files\0*.png\0All Files\0*.*\0";//L"All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        return true;
    }
    return false;
}

// Função para ler o conteúdo do arquivo binário
bool ReadFileContent(const std::wstring& filePath, std::vector<BYTE>& fileContent)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
        return false;

    // Obter o tamanho do arquivo
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Redimensionar o vetor para conter os dados do arquivo
    fileContent.resize(fileSize);

    // Ler os dados do arquivo
    file.read(reinterpret_cast<char*>(fileContent.data()), fileSize);

    file.close();

    return true;
}

// Função para converter dados binários para base64
std::wstring Base64Encode(const std::vector<BYTE>& data)
{
    static const wchar_t* base64_chars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::wstring base64;
    size_t input_size = data.size();
    const unsigned char* bytes_to_encode = data.data();
    size_t i = 0;
    unsigned char byte_array_3[3];
    unsigned char byte_array_4[4];

    while (input_size--)
    {
        byte_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            byte_array_4[0] = (byte_array_3[0] & 0xfc) >> 2;
            byte_array_4[1] = ((byte_array_3[0] & 0x03) << 4) + ((byte_array_3[1] & 0xf0) >> 4);
            byte_array_4[2] = ((byte_array_3[1] & 0x0f) << 2) + ((byte_array_3[2] & 0xc0) >> 6);
            byte_array_4[3] = byte_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                base64 += base64_chars[byte_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (size_t j = i; j < 3; j++)
            byte_array_3[j] = '\0';

        byte_array_4[0] = (byte_array_3[0] & 0xfc) >> 2;
        byte_array_4[1] = ((byte_array_3[0] & 0x03) << 4) + ((byte_array_3[1] & 0xf0) >> 4);
        byte_array_4[2] = ((byte_array_3[1] & 0x0f) << 2) + ((byte_array_3[2] & 0xc0) >> 6);

        for (size_t j = 0; j < i + 1; j++)
            base64 += base64_chars[byte_array_4[j]];

        while (i++ < 3)
            base64 += L'=';
    }

    return base64;
}

// Definindo variáveis globais:

HWND BTN_SELECT;
HWND MAIN_MSG;

int width, height;

int PosXIMG = 0;
int PosYIMG = 0;

static Image* image = NULL;

// Prototipo da função
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Função principal do Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Inicializa o GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Define e registra a classe da janela
    WNDCLASS wndClass = {};
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = L"ImageWindowClass";
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wndClass);

    // Cria a janela
    HWND hWnd = CreateWindowEx(
        0,
        L"ImageWindowClass",
        L"ConverterPNG_B64",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    // Exibe a janela
    ShowWindow(hWnd, nCmdShow);

    // Loop de mensagens
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Desliga o GDI+
    GdiplusShutdown(gdiplusToken);
    return 0;
}

// Função de processamento de mensagens da janela
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //static Image* image = NULL;
    //static int PosXIMG = 0, PosYIMG = 0;

    switch (message)
    {
    case WM_CREATE:
    {
        // Carrega a imagem PNG
        image = new Image(L"Img.png");

        BTN_SELECT = CreateWindowW(L"BUTTON", L"Selecionar", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 50, 100, 30, hWnd, (HMENU)1, nullptr, nullptr);

        HFONT FNT001 = CreateFont(36, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");

        MAIN_MSG = CreateWindowEx(0, L"STATIC", L"Now, you can convert your .PNG files to BASE64.", WS_CHILD | WS_VISIBLE, 0, 0, 600, 50, hWnd, NULL, GetModuleHandle(NULL), NULL);

        SendMessage(MAIN_MSG, WM_SETFONT, (WPARAM)FNT001, TRUE);
    }
    break;
    case WM_SIZE:
    {
        // PROPÓSITO DO SCRIPT:

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;

        int PosXMAIN_MSG = -300 + (windowWidth - width) / 2;
        int PosYMAIN_MSG = (windowHeight - height) / 5;

        SetWindowPos(MAIN_MSG, NULL, PosXMAIN_MSG, PosYMAIN_MSG, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        int PosXBTN_SELECT = -50 + (windowWidth - width) / 2;
        int PosYBTN_SELECT = 4 * ((windowHeight - height) / 5);

        SetWindowPos(BTN_SELECT, NULL, PosXBTN_SELECT, PosYBTN_SELECT, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        PosXIMG = 300;// (windowWidth - image->GetWidth()) / 2;
        PosYIMG = 0;

        // Solicita a repintura da janela
        InvalidateRect(hWnd, NULL, TRUE);
    }
    break;
    case WM_CTLCOLORSTATIC:
    {
        // PROPÓSITO DO SCRIPT: DEFINIR A COR DAS LETRAS E A COR DO FUNDO DAS ETIQUETAS.

        SetTextColor((HDC)wParam, RGB(0, 0, 0));
        SetBkColor((HDC)wParam, RGB(255, 255, 255));
        return (LRESULT)CreateSolidBrush(RGB(255, 255, 255));

    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Cria um Graphics a partir do HDC
        Graphics graphics(hdc);

        // Desenha a imagem
        if (image)
        {
            graphics.DrawImage(image, PosXIMG, PosYIMG, image->GetWidth(), image->GetHeight());
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
    {
        // Libera a imagem
        delete image;
        PostQuitMessage(0);
    }
    break;
    //
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1) // ID do botão é 1
        {
            // Abrir a caixa de diálogo de seleção de arquivo
            WCHAR selectedFile[MAX_PATH] = { 0 };
            if (OpenFileDialog(hWnd, selectedFile))
            {
                // Verificar se o arquivo selecionado é um arquivo PNG
                std::wstring filePath(selectedFile);
                std::wstring fileExtension = filePath.substr(filePath.find_last_of(L".") + 1);
                std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::towlower);

                if (fileExtension == L"png")
                {
                    // Extrair o nome do arquivo e a extensão
                    std::wstring fileName = filePath.substr(filePath.find_last_of(L"\\/") + 1);
                    std::wstring fileBaseName = fileName.substr(0, fileName.find_last_of(L"."));
                    std::wstring fileExt = fileName.substr(fileName.find_last_of(L".") + 1);

                    // Ler o conteúdo do arquivo
                    std::vector<BYTE> fileContent;
                    if (ReadFileContent(filePath, fileContent))
                    {
                        // Converter o conteúdo para base64
                        std::wstring base64Data = Base64Encode(fileContent);

                        // Salvar base64 em um arquivo texto com o nome do arquivo original
                        std::wofstream outFile(fileBaseName + L"_base64.txt");
                        if (outFile.is_open())
                        {
                            outFile << base64Data;
                            outFile.close();
                            MessageBox(hWnd, L"Arquivo PNG convertido para base64 e salvo.", L"Sucesso", MB_OK | MB_ICONINFORMATION);
                        }
                        else
                        {
                            MessageBox(hWnd, L"Falha ao criar o arquivo texto.", L"Erro", MB_OK | MB_ICONERROR);
                        }
                    }
                    else
                    {
                        MessageBox(hWnd, L"Falha ao ler o conteúdo do arquivo.", L"Erro", MB_OK | MB_ICONERROR);
                    }
                }
                else
                {
                    MessageBox(hWnd, L"Por favor, selecione um arquivo PNG.", L"Formato Inválido", MB_OK | MB_ICONWARNING);
                }
            }
        }
    }
    break;
    //
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

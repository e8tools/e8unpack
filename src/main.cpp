/*----------------------------------------------------------
This Source Code Form is subject to the terms of the 
Mozilla Public License, v.2.0. If a copy of the MPL 
was not distributed with this file, You can obtain one 
at http://mozilla.org/MPL/2.0/.
----------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
//	Author:			disa_da
//	E-mail:			disa_da2@mail.ru
/////////////////////////////////////////////////////////////////////////////

/**
    2014-2022       dmpas       sergey(dot)batanov(at)dmpas(dot)ru
    2019-2020       fishca      fishcaroot(at)gmail(dot)com
 */

// main.cpp : Defines the entry point for the console application.
//

#include "V8File.h"
#include "FilePath.h"
#include "version.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include "ConsoleOutput.h"               // для кириллицы в Windows
#include <boost/filesystem/fstream.hpp>

#ifdef _WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winnls.h>
#  ifndef CP_UTF8
#    define CP_UTF8 65001
#  endif
#endif

using namespace std;
using namespace v8unpack;

typedef int (*handler_t)(vector<string> &argv);
void read_param_file(const char *filename, vector< vector<string> > &list);
handler_t get_run_mode(const vector<string> &args, int &arg_base, bool &allow_listfile);

// Обработчики режимов: объявлены заранее, чтобы required_args_for()
// мог сравнивать указатели на функции.
int usage(vector<string> &argv);
int version(vector<string> &argv);
int inflate(vector<string> &argv);
int deflate(vector<string> &argv);
int unpack(vector<string> &argv);
int pack(vector<string> &argv);
int parse(vector<string> &argv);
int decompile(vector<string> &argv);
int build(vector<string> &argv);
int build_nopack(vector<string> &argv);
int compile(vector<string> &argv);
int list_files(vector<string> &argv);

static std::atomic_bool g_verbose{false};

static void consoleMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{

    // Отфильтровываем отладочные сообщения, если verbose не включён
    if (type == QtDebugMsg && !g_verbose.load(std::memory_order_relaxed))
        return;

    const char* prefix = "";
    switch (type) {
        case QtDebugMsg:    prefix = "[D] "; break;
        case QtInfoMsg:     prefix = "[I] "; break;
        case QtWarningMsg:  prefix = "[W] "; break;
        case QtCriticalMsg: prefix = "[C] "; break;
        case QtFatalMsg:    prefix = "[F] "; break;
    }
    v8unpack::writeStderr(QString::fromLatin1(prefix) + msg + QLatin1Char('\n'));
}


int usage(vector<string> &argv)
{
	cout << endl;
	cout << "V8Upack Version " << V8P_VERSION
		 << " Copyright (c) " << V8P_RIGHT << endl;

	cout << endl;
	cout << "Unpack, pack, deflate and inflate 1C v8 file (*.cf)" << endl;
	cout << endl;
	cout << "V8UNPACK" << endl;
	cout << "  -U[NPACK]            in_filename.cf     out_dirname [block_name]" << endl;
	cout << "  -U[NPACK]  -L[IST]   listfile" << endl;
	cout << "  -PA[CK]              in_dirname         out_filename.cf" << endl;
	cout << "  -PA[CK]    -L[IST]   listfile" << endl;
	cout << "  -I[NFLATE]           in_filename.data   out_filename" << endl;
	cout << "  -I[NFLATE] -L[IST]   listfile" << endl;
	cout << "  -D[EFLATE]           in_filename        filename.data" << endl;
	cout << "  -D[EFLATE] -L[IST]   listfile" << endl;
	cout << "  -P[ARSE]             in_filename        out_dirname [block_name1 block_name2 ...]" << endl;
	cout << "  -DE[COMPILE]         in_filename        out_dirname [block_name1 block_name2 ...]" << endl;
	cout << "  -P[ARSE]   -L[IST]   listfile" << endl;
	cout << "  -B[UILD] [-N[OPACK]] in_dirname         out_filename" << endl;
	cout << "  -CO[MPILE]           in_dirname         out_filename" << endl;
	cout << "  -B[UILD] [-N[OPACK]] -L[IST] listfile" << endl;
	cout << "  -L[IST]              listfile" << endl;
	
	cout << "  -LISTFILES|-LF       in_filename" << endl;

	cout << "  -E[XAMPLE]" << endl;
	cout << "  -BAT" << endl;
	cout << "  -V[ERSION]" << endl;

	return 0;
}

int version(vector<string> &argv)
{
	cout << V8P_VERSION << endl;
	return 0;
}

// Сколько непустых аргументов (считая с argv[0]) требует каждый режим.
// Возвращает -1, если режим определяет требования сам.
static int required_args_for(handler_t handler)
{
	if (handler == inflate)   return 2;   // in_filename out_filename
	if (handler == deflate)   return 2;   // in_filename out_filename
	if (handler == unpack)    return 2;   // in_filename out_dirname [необязательный block_name]
	if (handler == pack)      return 2;   // in_dirname out_filename
	if (handler == parse)     return 2;   // in_filename out_dirname [блоки...]
	if (handler == decompile) return 2;   // in_filename out_dirname [блоки...]
	if (handler == build)     return 2;   // in_dirname out_filename
	if (handler == build_nopack) return 2;
	if (handler == compile)   return 2;   // in_dirname out_filename
	if (handler == list_files) return 1;  // in_filename
	return -1;
}

// Проверить, что первые `required` аргументов существуют и непусты.
static bool has_args(const vector<string> &args, int required)
{
	if (required < 0) {
		return true;
	}

	const size_t count = static_cast<size_t>(required);
	if (args.size() < count) {
		return false;
	}

	for (size_t i = 0; i < count; i++) {
		if (args[i].empty()) {
			return false;
		}
	}

	return true;
}

int inflate(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(inflate))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = Inflate(argv[0], argv[1]);
	return ret;
}

int deflate(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(deflate))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = Deflate(argv[0], argv[1]);
	return ret;
}

int unpack(vector<string> &argv)
{
	// Третий аргумент (имя блока) необязателен: пустое значение означает
	// распаковку всего контейнера (см. unpack_to_folder в V8File.cpp).
	if (!has_args(argv, required_args_for(unpack))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = UnpackToFolder(argv[0], argv[1], argv[2], true);
	return ret;
}

int pack(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(pack))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = PackFromFolder(argv[0], argv[1]);
	return ret;
}

int parse(vector<string> &argv)
{

	if (!has_args(argv, required_args_for(parse))) {
		return V8UNPACK_SHOW_USAGE;
	}

	vector<string> filter;
	for (size_t i = 2; i < argv.size(); i++) {
		if (!argv[i].empty()) {
			filter.push_back(argv[i]);
		}
	}

	return Parse(argv[0], argv[1], filter);
}

int list_files(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(list_files))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = ListFiles(argv[0]);
	return ret;
}

int process_list(vector<string> &argv)
{
	if (argv.empty() || argv.at(0).empty()) {
		return V8UNPACK_SHOW_USAGE;
	}

	vector< vector<string> > commands;
	read_param_file(argv.at(0).c_str(), commands);

	for (auto command : commands) {

		int arg_base = 0;
		bool allow_listfile = false;

		handler_t handler = get_run_mode(command, arg_base, allow_listfile);

		command.erase(command.begin());
		int ret = handler(command);
		if (ret != 0) {
			// выходим по первой ошибке
			return ret;
		}
	}

	return 0;
}

int bat(vector<string> &argv)
{
	cout << "if %1 == P GOTO PACK" << endl;
	cout << "if %1 == p GOTO PACK" << endl;
	cout << "" << endl;
	cout << "" << endl;
	cout << ":UNPACK" << endl;
	cout << "V8Unpack.exe -unpack      %2                              %2.unp" << endl;
	cout << "V8Unpack.exe -undeflate   %2.unp\\metadata.data            %2.unp\\metadata.data.und" << endl;
	cout << "V8Unpack.exe -unpack      %2.unp\\metadata.data.und        %2.unp\\metadata.unp" << endl;
	cout << "GOTO END" << endl;
	cout << "" << endl;
	cout << "" << endl;
	cout << ":PACK" << endl;
	cout << "V8Unpack.exe -pack        %2.unp\\metadata.unp            %2.unp\\metadata_new.data.und" << endl;
	cout << "V8Unpack.exe -deflate     %2.unp\\metadata_new.data.und   %2.unp\\metadata.data" << endl;
	cout << "V8Unpack.exe -pack        %2.unp                         %2.new.cf" << endl;
	cout << "" << endl;
	cout << "" << endl;
	cout << ":END" << endl;

	return 0;
}

int example(vector<string> &argv)
{
	cout << "" << endl;
	cout << "" << endl;
	cout << "UNPACK" << endl;
	cout << "V8Unpack.exe -unpack      1Cv8.cf                         1Cv8.unp" << endl;
	cout << "V8Unpack.exe -undeflate   1Cv8.unp\\metadata.data          1Cv8.unp\\metadata.data.und" << endl;
	cout << "V8Unpack.exe -unpack      1Cv8.unp\\metadata.data.und      1Cv8.unp\\metadata.unp" << endl;
	cout << "" << endl;
	cout << "" << endl;
	cout << "PACK" << endl;
	cout << "V8Unpack.exe -pack        1Cv8.unp\\metadata.unp           1Cv8.unp\\metadata_new.data.und" << endl;
	cout << "V8Unpack.exe -deflate     1Cv8.unp\\metadata_new.data.und  1Cv8.unp\\metadata.data" << endl;
	cout << "V8Unpack.exe -pack        1Cv8.und                        1Cv8_new.cf" << endl;
	cout << "" << endl;
	cout << "" << endl;

	return 0;
}

int build(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(build))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = BuildCfFile(argv[0], argv[1], false);
	return ret;
}

int build_nopack(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(build_nopack))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = BuildCfFile(argv[0], argv[1], true);
	return ret;
}

// -DE[COMPILE]  in_filename  out_dirname  [block_name1 block_name2 ...]
// Логика аналогична parse: разбор файла в каталог с необязательным фильтром по блокам.
// Происходит разбор полученных файлов по метаданным
int decompile(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(decompile))) {
		return V8UNPACK_SHOW_USAGE;
	}

	vector<string> filter;
	for (size_t i = 2; i < argv.size(); i++) {
		if (!argv[i].empty()) {
			filter.push_back(argv[i]);
		}
	}

	return ParseDecompile(argv[0], argv[1], filter);
}

// -CO[MPILE]  in_dirname  out_filename
// Логика аналогична build без опции -N[OPACK].
// Файлы собираются во временный каталог, потом из него собирается конфигурация
int compile(vector<string> &argv)
{
	if (!has_args(argv, required_args_for(compile))) {
		return V8UNPACK_SHOW_USAGE;
	}

	int ret = BuildCfFileCompile(argv[0], argv[1], false);
	return ret;
}

handler_t get_run_mode(const vector<string> &args, int &arg_base, bool &allow_listfile)
{
	if (args.size() - arg_base < 1) {
		allow_listfile = false;
		return usage;
	}

	allow_listfile = true;
	string cur_mode(args[arg_base]);
	transform(cur_mode.begin(), cur_mode.end(), cur_mode.begin(), ::tolower);

	arg_base += 1;
	if (cur_mode == "-version" || cur_mode == "-v") {
		allow_listfile = false;
		return version;
	}

	if (cur_mode == "-inflate" || cur_mode == "-i" || cur_mode == "-und" || cur_mode == "-undeflate") {
		return inflate;
	}

	if (cur_mode == "-deflate" || cur_mode == "-d") {
		return deflate;
	}
	
	if (cur_mode == "-decompile" || cur_mode == "-de") {
		return decompile;
	}

	if (cur_mode == "-unpack" || cur_mode == "-u" || cur_mode == "-unp") {
		return unpack;
	}

	if (cur_mode == "-pack" || cur_mode == "-pa") {
		return pack;
	}

	if (cur_mode == "-parse" || cur_mode == "-p") {
		return parse;
	}

	if (cur_mode == "-build" || cur_mode == "-b") {

		bool dont_pack = false;

		while ((int)args.size() > arg_base) {
			string arg2(args[arg_base]);
			transform(arg2.begin(), arg2.end(), arg2.begin(), ::tolower);
			if (arg2 == "-n" || arg2 == "-nopack") {
				arg_base++;
				dont_pack = true;
			} else {
				break;
			}
		}
		return dont_pack ? build_nopack : build;
	}

	if (cur_mode == "-compile" || cur_mode == "-co") {
		return compile;
	}

	allow_listfile = false;
	if (cur_mode == "-bat") {
		return bat;
	}

	if (cur_mode == "-example" || cur_mode == "-e") {
		return example;
	}

	if (cur_mode == "-list" || cur_mode == "-l") {
		return process_list;
	}

	if (cur_mode == "-listfiles" || cur_mode == "-lf") {
		return list_files;
	}

	return nullptr;
}

void read_param_file(const char *filename, vector< vector<string> > &list)
{
	boost::filesystem::ifstream in(to_path(filename));
	string line;
	while (getline(in, line)) {

		vector<string> current_line;

		stringstream ss;
		ss.str(line);

		string item;
		while (getline(ss, item, ';')) {
			current_line.push_back(item);
		}

		while (current_line.size() < 5) {
			// Дополним пустыми строками, чтобы избежать лишних проверок
			current_line.emplace_back("");
		}

		list.push_back(current_line);
	}
}

int main(int argc, char* argv[])
{
	int arg_base = 1;
	bool allow_listfile = false;
	vector<string> args;
	
	qInstallMessageHandler(consoleMessageHandler);

	// Включаем диагностику сразу после разбора, чтобы все последующие
	// qDebug() из модулей были видны (или скрыты) корректно.
	g_verbose.store(true, std::memory_order_relaxed);


	// Командная строка Windows приходит в ANSI-кодировке процесса: кириллица
	// в путях терялась бы ещё до входа в main(). Забираем её в UTF-8.
	args = command_line_args(argc, argv);
	handler_t handler = get_run_mode(args, arg_base, allow_listfile);

	vector<string> cli_args;

	if (handler == nullptr || handler == usage) {
		usage(cli_args);
		return 1;
	}

	// Аргументы командной строки после режима.
	// Заполняем до размера, который читают обработчики, чтобы любое
	// обращение по индексу оставалось внутри вектора: за реальные
	// аргументы отвечает проверка has_args() внутри обработчиков.
	for (size_t i = static_cast<size_t>(arg_base); i < args.size(); i++) {
		cli_args.push_back(args[i]);
	}
	while (cli_args.size() < 3) {
		cli_args.emplace_back("");
	}

	if (allow_listfile && (cli_args[0] == "-list" || cli_args[0] == "-l")) {
		// Передан файл с параметрами
		if (cli_args[1].empty()) {
			usage(cli_args);
			return V8UNPACK_SHOW_USAGE;
		}

		vector< vector<string> > param_list;
		read_param_file(cli_args[1].c_str(), param_list);

		int ret = 0;

		for (auto argv_from_file : param_list) {
			int ret1 = handler(argv_from_file);
			if (ret1 != 0 && ret == 0) {
				ret = ret1;
			}
		}

		return ret;
	}

	int ret = handler(cli_args);
	if (ret == V8UNPACK_SHOW_USAGE) {
		usage(cli_args);
	}
	return ret;
}

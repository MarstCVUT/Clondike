require 'mkmf'

dir_config("directorApi")
find_header("director-api.h", "../director-api/")
#find_library("director-api", "initialize_director_api", "../director-api/") WTF this does not work?
$LDFLAGS += " -L../director-api/"
$libs = append_library($libs, "director-api")
$libs = append_library($libs, "nl")
create_makefile("directorApi")
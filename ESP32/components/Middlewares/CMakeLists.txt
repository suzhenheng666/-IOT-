set(src_dirs
			Protocol
			WIFI)

set(include_dirs
			Protocol
			WIFI)

set(requires
			driver
			log
			esp_wifi
			esp_event
			nvs_flash)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})
component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)

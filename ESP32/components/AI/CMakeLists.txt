idf_component_register(
    SRCS "yolo_detect.c"
    INCLUDE_DIRS "."
    REQUIRES esp-dl
    EMBED_FILES "fault_light_model.espdl"
)

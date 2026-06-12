cmake_minimum_required(VERSION 3.20)

# Protobuf sources
file(GLOB PROTO_SRCS "../../proto/*.proto")
nanopb_generate_cpp(
	TARGET proto
	RELPATH "../../proto/"
	${PROTO_SRCS}
)

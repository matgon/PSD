

int main (int argc, char **argv){

	register SVCXPRT *transp;

	pmap_unset (CONECTA4, CONECTA4VER);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}

	//da error aqui
	if (!svc_register(transp, CONECTA4, CONECTA4VER, conecta4_1, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (CONECTA4, CONECTA4VER, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}

	if (!svc_register(transp, CONECTA4, CONECTA4VER, conecta4_1, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (CONECTA4, CONECTA4VER, tcp).");
		exit(1);
	}

	// Init data structures
	initServerStructures ();
	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
}

//program controls ptu2f3
//gcc -o ptu2f3 main.c -lusb
#define VERSION "0.3"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <usb.h>
#include <unistd.h>

//Define
#define USB_VENDOR 0x0711
#define USB_PRODUCT 0x0028
#define TIMEOUT (5*1000)

#define MAX_NUM_DEVICE 128

/* Init USB */
struct usb_bus *USB_init()
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	return(usb_get_busses());
}

/* Find USB device  */
int USB_find(struct usb_bus *busses, struct usb_device *dev_arr[])
{
	struct usb_device *dev;
	struct usb_bus *bus;
	int dev_idx=0;
	for(bus=busses; bus; bus=bus->next){
		for(dev=bus->devices; dev; dev=dev->next) {
			if( (dev->descriptor.idVendor==USB_VENDOR) && (dev->descriptor.idProduct==USB_PRODUCT) ){
				dev_arr[dev_idx]=dev;dev_idx++;
				if(dev_idx>=MAX_NUM_DEVICE)break;
			}
		}
	}
	return dev_idx;
}

/* USB Open */
struct usb_dev_handle *USB_open(struct usb_device *dev)
{
	struct usb_dev_handle *udev = NULL;

	udev=usb_open(dev);
	if( (udev=usb_open(dev))==NULL ){
		fprintf(stderr,"usb_open Error.(%s)\n",usb_strerror());
		exit(1);
	}

	if( usb_set_configuration(udev,dev->config->bConfigurationValue)<0 ){
		if( usb_detach_kernel_driver_np(udev,dev->config->interface->altsetting->bInterfaceNumber)<0 ){
			fprintf(stderr,"usb_set_configuration Error.\n");
			fprintf(stderr,"usb_detach_kernel_driver_np Error.(%s)\n",usb_strerror());
 		}
	}

	if( usb_claim_interface(udev,dev->config->interface->altsetting->bInterfaceNumber)<0 ){
		if( usb_detach_kernel_driver_np(udev,dev->config->interface->altsetting->bInterfaceNumber)<0 ){
			fprintf(stderr,"usb_claim_interface Error.\n");
			fprintf(stderr,"usb_detach_kernel_driver_np Error.(%s)\n",usb_strerror());
		}
		}

	if( usb_claim_interface(udev,dev->config->interface->altsetting->bInterfaceNumber)<0 ){
		fprintf(stderr,"usb_claim_interface Error.(%s)\n",usb_strerror());
	}

	return(udev);
}

/* USB Close */
void USB_close(struct usb_dev_handle *dh)
{
	if(usb_release_interface(dh, 0)){
		fprintf(stderr,"usb_release_interface() failed. (%s)\n",usb_strerror());
	}
	if( usb_close(dh)<0 ){
		fprintf(stderr,"usb_close Error.(%s)\n",usb_strerror());
	}
}

/* USB altinterface */
void USB_altinterface(struct usb_dev_handle *dh,int alternate)
{
	if(usb_set_altinterface(dh,alternate)<0)
	{
		fprintf(stderr,"Failed to set altinterface %d: %s\n", 1,usb_strerror());
		USB_close(dh);
	}
}

void usage(void){
	printf("PTU2F3 USB AC tap controller, version %s\n",VERSION);
	printf("Usage: ptu2f3 [option] [4|5|45] [ON|OFF]\n");
	printf(" -h         : This message\n");
	printf(" -m [0-%d] : Device to control (Deault 0)\n",MAX_NUM_DEVICE-1);
	printf(" -s         : Status report\n");
//	printf(" -d         : Debug message on\n");
	exit(0);
}

int debug=0;

void debug_msg(unsigned char *msg){
	if(debug==1){
		printf("\n%s\n",msg);
	}
}

unsigned char get_status(usb_dev_handle *dh,int display_msg){
	unsigned char readStatus[8] =  {1,0,0,0,0,0,0,0};
	unsigned char msg[100];
	unsigned char display[100];

	unsigned char port4s,port5s,port_status;
	port4s=port5s=port_status=0;

	int result=usb_bulk_write(dh, 0x02, readStatus, sizeof(readStatus), TIMEOUT);
	if(result<0){fprintf (stderr,"Control message error. (%d:%s)\n", result, usb_strerror());}

	debug_msg("status...\n");
	memset(readStatus, 0, sizeof(unsigned char)*9);
	result = usb_bulk_read(dh, 0x81, readStatus, sizeof(readStatus), TIMEOUT);
	if(result<0){fprintf (stderr,"Control message error. (%d:%s)\n", result, usb_strerror());}

	sprintf(msg,"readStatus: %d %d %d\n", readStatus[0],readStatus[1],readStatus[2]);debug_msg(msg);
	sprintf(msg,"readStatus(4,5): %d %d,port_status: %d\n", ~readStatus[0]&1,~readStatus[0]&4,port_status);debug_msg(msg);
	sprintf(display,"Port 4:%s, Port 5:%s",((~readStatus[0]&1)>0)?"ON":"OFF",((~readStatus[0]&4)>0)?"ON":"OFF");

	if(display_msg==1){
		printf(display);
		printf("\n");
	}

	return readStatus[0];
}

void main(int argc, char *argv[])
{
	struct usb_bus *bus;
	struct usb_device *dev;
	struct usb_device *dev_arr[MAX_NUM_DEVICE];
	usb_dev_handle *dh,*dh1;
	int num_dev=0;

	unsigned char writeStatus[8] = {2,0,0,0,0,0,0,0};
	unsigned char readStatus[8] =  {1,0,0,0,0,0,0,0};
	unsigned char msg[100];

	unsigned char *arg1;

	int	ch,control,status,verbose,result;
	unsigned char port4,port5,current_port_status,new_status;
	int dev_val=0;int i;
	extern char	*optarg;
	extern int optind, opterr;

	control=1;
	status=debug=0;
	while ((ch = getopt(argc, argv, "hsdm:")) != -1){
		switch (ch){
			case 's':
				status=1;control=0;
				break;
			case 'd':
				debug=1;
				break;
			case 'm':
				dev_val = strtod(optarg, NULL);
				break;
			case 'h':
			default:
				control=0;
				usage();
		}
	}
	argc -= optind;
	argv += optind;

	if(control>0 || status>0){
		if(control==1 && argc!=2)usage();

		/* Initialize */
		bus=USB_init();
		num_dev=USB_find(bus,dev_arr);
		if( num_dev==0 ){
			fprintf(stderr,"Device not found\n");
			exit(1); 
		}
		sprintf(msg,"Initialize OK\n");debug_msg(msg);
		/*-------------*/
		/* Device Open */
		/*-------------*/
		if(dev_val+1>num_dev){
			fprintf(stderr,"Invalid device index\n");
			exit(1); 
		}
		dev=dev_arr[dev_val];
		dh=USB_open(dev);
		if( dh==NULL ){ exit(2); }
		sprintf(msg,"Device Open OK\n");debug_msg(msg);

		if(control==1 && argc==2){

			current_port_status=get_status(dh,0);
			port4=((~current_port_status&1)>0)?1:0;
			port5=((~current_port_status&4)>0)?1:0;

			sprintf(msg,"current_port_status: %u\n", current_port_status);debug_msg(msg);
			sprintf(msg,"port4,port5: %u %u\n", port4,port5);debug_msg(msg);

			if(strncmp(argv[0],"45",2)==0 && strncmp(argv[1],"OFF",3)==0){
				port4=0;port5=0;
			}else if(strncmp(argv[0],"45",2)==0 && strncmp(argv[1],"ON",2)==0){
				port4=1;port5=1;
			}else if(strncmp(argv[0],"4",1)==0 && strncmp(argv[1],"OFF",3)==0){
				port4=0;
			}else if(strncmp(argv[0],"4",1)==0 && strncmp(argv[1],"ON",2)==0){
				port4=1;
			}else if(strncmp(argv[0],"5",1)==0 && strncmp(argv[1],"OFF",3)==0){
				port5=0;
			}else if(strncmp(argv[0],"5",1)==0 && strncmp(argv[1],"ON",2)==0){
				port5=1;
			}
			sprintf(msg,"port4: %d, port5: %d \n", port4,port5);debug_msg(msg);

			new_status=(port5>0?0:1)*2+(port4>0?0:1);
			sprintf(msg,"new_status: %u\n", new_status);debug_msg(msg);
			writeStatus[1]=new_status;

			sprintf(msg,"writeStatus: %d %d %d\n", writeStatus[0],writeStatus[1],writeStatus[2]);debug_msg(msg);
			result=usb_bulk_write(dh, 0x02, writeStatus, sizeof(writeStatus), TIMEOUT);
			if(result<0){printf ("Control message error. (%d:%s)\n", result, usb_strerror());}

			printf("Device %d => ",dev_val);get_status(dh,1);
		}

		debug_msg("USB Close\n");
		USB_close(dh);
		debug_msg("USB End\n");

		if(status==1){
			printf("%d devices found\n",num_dev);
			for(i=0;i<num_dev;i++){
			dh1=USB_open(dev_arr[i]);
			printf("Device %d => ",i);get_status(dh1,1);
			USB_close(dh1);
			}
		}
	

	}else{
		usage();
	}
	exit(0);
}

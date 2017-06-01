#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <rtl_json.h>

int main()
{
	rtl_json_t *root = rtl_json_create_object();
	printf("%d\n", rtl_json_get_array_size(root));

	char *str = rtl_json_print(root);
	printf("%s\n", str);
	free(str);

	rtl_json_add_number_to_object(root, "type", 1);
	rtl_json_add_string_to_object(root, "mac", "00117F934A02");
	rtl_json_add_string_to_object(root, "ip", "192.168.1.1");

	str = rtl_json_print_unformatted(root);
	printf("%s\n", str);
	printf("%d\n", rtl_json_get_array_size(root));

	free(str);

	str = rtl_json_print(root);
	printf("%s\n", str);

	free(str);

	rtl_json_delete(root);

	return 0;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI_Handler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ashirzad <ashirzad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/12 12:23:04 by ashirzad          #+#    #+#             */
/*   Updated: 2025/02/12 12:47:27 by ashirzad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <map>
#include <cstdlib>
#include <sys/types.h>
#include <vector>
#include <cstring>

class CGIHandler
{
	private :
		std::map<std::string, std::string> _env;
	public :
		CGIHandler(void);
		~CGIHandler(void);

		char **getEnvAsCstrArray(void);
		void executeCGI(void);
};

#endif

/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef NODESETCOMMAND_H
#define NODESETCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "node set" command.
 *
 * @ingroup cli
 */
class NodeSetCommand : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(NodeSetCommand);

	virtual String GetDescription(void) const;
	virtual String GetShortDescription(void) const;
	virtual int GetMinArguments(void) const;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
	    boost::program_options::options_description& hiddenDesc) const;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const;
};

}

#endif /* NODESETCOMMAND_H */

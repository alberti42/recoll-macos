#ifndef _hldata_h_included_
#define _hldata_h_included_

#include <vector>
#include <string>
#include <set>

/** Store about user terms and their expansions. This is used mostly for
 *  highlighting result text and walking the matches.
 */
struct HighlightData {
    /** The user terms, excluding those with wildcards. 
     * This list is intended for orthographic suggestions but the terms are
     * unaccented lowercased anyway because they are compared to the dictionary
     * generated from the index term list (which is unaccented).
     */
    std::set<std::string> uterms;

    /** The original user terms-or-groups. This is for displaying the matched
     * terms or groups, ie in relation with highlighting or skipping to the 
     * next match. These are raw, diacritics and case preserved.
     */
    std::vector<std::vector<std::string> > ugroups;

    /** Processed/expanded terms and groups. Used for looking for
     * regions to highlight. Terms are just groups with 1 entry. All
     * terms are transformed to be compatible with index content
     * (unaccented and lowercased as needed depending on
     * configuration), and the list may include values
     * expanded from the original terms by stem or wildcard expansion.
     */
    std::vector<std::vector<std::string> > groups;
    /** Group slacks. Parallel to groups */
    std::vector<int> slacks;

    /** Index into ugroups for each group. Parallel to groups. As a
     * user term or group may generate many processed/expanded terms
     * or groups, this is how we relate them 
     */
    std::vector<unsigned int> grpsugidx;

    void clear()
    {
	uterms.clear();
	ugroups.clear();
	groups.clear();
	slacks.clear();
	grpsugidx.clear();
    }
    void append(const HighlightData&);

    // Print (debug)
    void toString(std::string& out);
};

#endif /* _hldata_h_included_ */

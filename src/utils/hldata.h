#ifndef _hldata_h_included_
#define _hldata_h_included_

#include <vector>
#include <string>
#include <set>
#include <map>

/** Store data about user search terms and their expansions. This is used
 * mostly for highlighting result text and walking the matches, generating 
 * spelling suggestions.
 */
struct HighlightData {
    /** The user terms, excluding those with wildcards. This list is
     * intended for orthographic suggestions so the terms are always
     * lowercased, unaccented or not depending on the type of index 
     * (as the spelling dictionary is generated from the index terms).
     */
    std::set<std::string> uterms;

    /** The db query terms linked to the uterms entry they were expanded from. 
     * This is used for aggregating term stats when generating snippets (for 
     * choosing the best terms, allocating slots, etc. )
     */
    std::map<std::string, std::string> terms;

    /** The original user terms-or-groups. This is for display
     * purposes: ie when creating a menu to look for a specific
     * matched group inside a preview window. We want to show the
     * user-entered data in the menu, not some transformation, so
     * these are always raw, diacritics and case preserved.
     */
    std::vector<std::vector<std::string> > ugroups;

    /** Processed/expanded terms and groups. Used for looking for
     * regions to highlight. A group can be a PHRASE or NEAR entry (we
     * process everything as NEAR to keep things reasonably
     * simple. Terms are just groups with 1 entry. All
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
     * or groups, this is how we relate an expansion to its source
     * (used, e.g. for generating anchors for walking search matches
     * in the preview window).
     */
    std::vector<size_t> grpsugidx;

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
